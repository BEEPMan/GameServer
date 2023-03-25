#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"

Room GRoom;

void Room::Enter(PlayerRef player)
{
	WRITE_LOCK;
	_players[player->playerId] = player;
}

void Room::Leave(PlayerRef player)
{
	WRITE_LOCK;
	_players.erase(player->playerId);
}

void Room::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (auto& p : _players)
	{
		p.second->ownerSession->Send(sendBuffer);
	}
}

void Room::BroadcastOthers(SendBufferRef sendBuffer, uint64 playerId)
{
	WRITE_LOCK;
	for (auto& p : _players)
	{
		if (p.first == playerId)
			continue;
		p.second->ownerSession->Send(sendBuffer);
	}
}