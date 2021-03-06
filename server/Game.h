#ifndef GAME
#define GAME

#include "Player.h"
#include "../shared/Card.h"
#include <vector>
#include <string>
#include <algorithm>
#include "debug.h"

#define TURN_TIME 25;

enum GAME_TYPE {
	GAME_NONE,
	GAME_LOBBY,
	GAME_GAME,
	GAME_OVER
};

class Game {
public:
	Game(int id);
	~Game();

	int GetId();
	int GetNumberOfPlayers();

	bool MakeLobby();
	bool IsLobby();
	bool SetGameHost(Player* player);
	Player *GetGameHost();

	bool MakeGame();
	void PrepareCards();
	void Shuffle();

	bool IsPlayerInGame(Player* player);
	bool AddPlayer(Player* player);
	bool RemovePlayer(Player* player);
	bool RemovePlayer(int playerid);
	int GetAmountOfPlayers();

	void CheckWinConditions();
	void Win(Player* player);
	bool IsGameOver();
	std::string SendGameOverMessage();

	Card* GetCardOnTop();

	std::string ExecuteMove(std::string datain);
	bool Validate(Card *card);
	void PassTurn();

	std::string MsgGetLobbyStatus();
	std::string MsgGetGameStatus(int playerid);

private:
	void AddDeck();
	void Deal();
	void TransferCardToPlayer(Card* card, Player* player);
	void TransferCardFromPlayer(Card* card, Player* player);
	Card* FindCardToPutOnTop();
	void PutOnTop(Card* card);
	void ExecuteFirstMove();

	int iGameHostId;

	int iType;
	int iId;

	long long int iTurnEndTime;

	std::vector<Player*> vPlayers;
	std::vector<Card*> vCards;
	std::vector<Card*> vDeck;
	Card* pCardOnTop;

	int iTurn;
	int iPlayerTurnId;

	int iPlayerWonId;

	int iToPull;
	int iANum;
	int iJNum;
};


#endif