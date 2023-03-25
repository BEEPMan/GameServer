#pragma once
class Player
{
public:

	uint64					playerId = 0;
	string					name;
	Protocol::PlayerType	type = Protocol::PLAYER_TYPE_NONE;
	float x = 0.0f;
	float y = 0.0f;
	GameSessionRef			ownerSession; // Cycle
};

