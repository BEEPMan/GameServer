#pragma once
class Room
{
public:
	void Enter(PlayerRef player);
	void Leave(PlayerRef player);
	void Broadcast(SendBufferRef sendBuffer);
	void BroadcastOthers(SendBufferRef sendBuffer, uint64 playerId);

public:
	USE_LOCK;
	map<uint64, PlayerRef> _players;
};

extern Room GRoom;