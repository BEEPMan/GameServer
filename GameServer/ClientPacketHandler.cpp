#include "pch.h"
#include "ClientPacketHandler.h"
#include "Player.h"
#include "Room.h"
#include "GameSession.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

// 직접 컨텐츠 작업자

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	cout << "INVALID Header" << endl;
	return true;
}

bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	// TODO : Validation Check
	cout << "Recv C_LOGIN" << endl;

	Protocol::S_LOGIN loginPkt;
	loginPkt.set_success(true);

	// DB에서 플레이어 정보를 긁어온다
	// GameSession에 플레이어의 정보를 저장 (메모리)

	// ID 발급 (DB 아이디가 아니고, 인게임 아이디)
	static Atomic<uint64> idGenerator = 1;

	for (auto& p : GRoom._players)
	{
		auto player = loginPkt.add_players();
		player->set_id(p.second->playerId);
		player->set_name(p.second->name);
		player->set_playertype(p.second->type);
		player->set_x(p.second->x);
		player->set_y(p.second->y);
	}

	{
		auto player = loginPkt.add_players();
		player->set_id(idGenerator);
		player->set_name(pkt.playername());
		player->set_playertype(Protocol::PLAYER_TYPE_KNIGHT);
		player->set_x(0.0f);
		player->set_y(0.0f);

		PlayerRef playerRef = MakeShared<Player>();
		playerRef->playerId = idGenerator;
		playerRef->name = player->name();
		playerRef->type = player->playertype();
		playerRef->ownerSession = gameSession;
		playerRef->x = 0.0f;
		playerRef->y = 0.0f;

		loginPkt.set_playerid(idGenerator++);

		gameSession->_players.push_back(playerRef);
		GRoom.Enter(playerRef);
	}

	/*{
		auto player = loginPkt.add_players();
		player->set_name(u8"DB에서긁어온이름2");
		player->set_playertype(Protocol::PLAYER_TYPE_MAGE);

		PlayerRef playerRef = MakeShared<Player>();
		playerRef->playerId = idGenerator++;
		playerRef->name = player->name();
		playerRef->type = player->playertype();
		playerRef->ownerSession = gameSession;

		cout << playerRef->playerId << endl;

		gameSession->_players.push_back(playerRef);
	}*/

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(loginPkt);

	session->Send(sendBuffer);

	return true;
}

bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
	cout << "Recv C_ENTER_GAME" << endl;

	Protocol::S_ENTER_GAME enterGamePkt;

	uint64 playerId = pkt.playerid();

	{
		PlayerRef playerRef = GRoom._players[playerId];
		Protocol::Player* player = new Protocol::Player();
		player->set_id(playerId);
		player->set_name(playerRef->name);
		player->set_playertype(playerRef->type);
		player->set_x(playerRef->x);
		player->set_y(playerRef->y);

		cout << playerRef->playerId << "_" << playerRef->name << " Entered the game." << endl;

		enterGamePkt.set_allocated_player(player);
	}

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(enterGamePkt);

	GRoom.BroadcastOthers(sendBuffer, pkt.playerid()); // WRITE_LOCK

	return true;
}

bool Handle_C_LEAVE_GAME(PacketSessionRef& session, Protocol::C_LEAVE_GAME& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	cout << "Recv C_LEAVE_GAME" << endl;

	Protocol::S_LEAVE_GAME leaveGamePkt;
	leaveGamePkt.set_playerid(pkt.playerid());

	PlayerRef player = GRoom._players[pkt.playerid()];
	cout << player->playerId << "_" << player->name << " Left the game." << endl;
	GRoom.Leave(player);

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(leaveGamePkt);

	GRoom.BroadcastOthers(sendBuffer, pkt.playerid()); // WRITE_LOCK

	return true;
}

//bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
//{
//	//GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
//
//	//uint64 index = pkt.playerindex();
//	//// TODO : Validation
//
//	//PlayerRef player = gameSession->_players[index]; // READ_ONLY?
//	//GRoom.Enter(player); // WRITE_LOCK
//
//	//Protocol::S_ENTER_GAME enterGamePkt;
//	//enterGamePkt.set_success(true);
//	//auto sendBuffer = ClientPacketHandler::MakeSendBuffer(enterGamePkt);
//	//player->ownerSession->Send(sendBuffer);
//
//	return true;
//}

bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt)
{
	cout << "Recv C_CHAT" << endl;
	std::cout << pkt.playerid() << "_" << GRoom._players[pkt.playerid()]->name << ": " << pkt.msg() << endl;

	Protocol::S_CHAT chatPkt;
	chatPkt.set_playerid(pkt.playerid());
	chatPkt.set_msg(pkt.msg());
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);

	GRoom.Broadcast(sendBuffer); // WRITE_LOCK

	return true;
}

bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	cout << "Recv C_MOVE" << endl;

	if (GRoom._players.find(pkt.playerid()) == GRoom._players.end())
		return true;
	GRoom._players[pkt.playerid()]->x = pkt.x();
	GRoom._players[pkt.playerid()]->y = pkt.y();

	Protocol::MoveInfo* moveInfo = new Protocol::MoveInfo(pkt.moveinfo());

	Protocol::S_MOVE movePkt;
	movePkt.set_playerid(pkt.playerid());
	movePkt.set_x(pkt.x());
	movePkt.set_y(pkt.y());
	movePkt.set_allocated_moveinfo(moveInfo);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(movePkt);

	GRoom.BroadcastOthers(sendBuffer, pkt.playerid()); // WRITE_LOCK

	return true;
}
