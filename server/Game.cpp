#include "Game.h"
#include <stdlib.h>
#include "SessionHandler.h"
#include <chrono>

Game::Game(int id) {
	iId = id;
	iType = GAME_NONE;
	iGameHostId = -1;
	iTurn = 1;
	iPlayerTurnId = -1;
	iTurnEndTime = -1;
	iPlayerWonId = -1;
	iANum = -1;
	iJNum = -1;
	iToPull = 1;
	pCardOnTop = nullptr;
}

int Game::GetId() {
	return iId;
}

int Game::GetNumberOfPlayers() {
	int n = 0;
	for (size_t i = 0; i < vPlayers.size(); i++) {
		n++;
	}
	return n;
}

bool Game::MakeLobby() {
	Msg("[G#" << GetId() << "] Demoting game to lobby");
	iType = GAME_LOBBY;
	Success("[G#" << GetId() << "] Game demoted to lobby!");
	return true; //?
}

bool Game::IsLobby() {
	if (iType == GAME_LOBBY) return true;
	else return false;
}

// returns true on success
bool Game::SetGameHost(Player* player) {
	if (!player) return false;
	iGameHostId = player->GetId();
	Msg("[G#" << GetId() << "] Setting host to player #" << iGameHostId);
	Success("[G#" << GetId() << "] Host set!");
	return true;
}

// returns Player* if game host is set
// returns NULL otherwise
Player* Game::GetGameHost() {
	for (size_t i = 0; i < vPlayers.size(); i++) {
		if (vPlayers[i]->GetId() == iGameHostId) return vPlayers[i];
	}
	return NULL;
}

bool Game::MakeGame() {
	Msg("[G#" << GetId() << "] Promoting lobby to game");
	iType = GAME_GAME;
	PrepareCards();
	Success("[G#" << GetId() << "] Lobby promoted to game!");
	return true;
}

void Game::PrepareCards() {
	//todo: count how many decks do we need.
	Msg("[G#" << GetId() << "] Preparing cards...");
	AddDeck();
	Shuffle();
	Deal();
	PutOnTop(FindCardToPutOnTop());
	ExecuteFirstMove();
}

void Game::Shuffle() {
	Msg("[G#" << GetId() << "] Shuffling cards...");
	std::random_shuffle(vDeck.begin(), vDeck.end());
}

bool Game::IsPlayerInGame(Player* player) {
	if (!player) return false;

	int pid = player->GetId();

	// check if the player is in the game
	for (size_t i = 0; i < vPlayers.size(); i++) {
		if (vPlayers[i]->GetId() == pid) return true; // return true if so
	}
	return false; // return false if not
}

// returns true on success or false on failure
bool Game::AddPlayer(Player *player) {
	Msg("[G#" << GetId() << "] Adding player to the game...");
	if (!player) {
		Error("[G#" << GetId() << "] Invalid player!");
		return false;
	}

	if (iType == GAME_GAME || iType == GAME_OVER) {
		Warn("[G#" << GetId() << "] Can't join the game, because it's currently in progress.");
		return false;
	}

	int pid = player->GetId();

	if (GetAmountOfPlayers() > 5) {
		Warn("[G#" << GetId() << "] Player limit in one game reached.");
		return false;
	}

	// check if the player isn't already in the game
	for (size_t i = 0; i < vPlayers.size(); i++) {
		if (vPlayers[i]->GetId() == pid) {
			Msg("[G#" << GetId() << "] Player already in game.");
			return false; // and return false if is
		}
	}

	vPlayers.push_back(player);
	Success("[G#" << GetId() << "] Player #" << player->GetId() << " added to the game");
	return true; // success. ...presumably...
}

//todo
bool Game::RemovePlayer(Player *player) {
	return true;
}

bool Game::RemovePlayer(int playerid) {
	Msg("[G#" << GetId() << "] Removing player from the game...");
	for (size_t i = 0; i < vPlayers.size(); i++) {
		if (vPlayers[i]->GetId() == playerid) vPlayers.erase(vPlayers.begin() + i);
	}
	Success("[G#" << GetId() << "] Player #" << playerid << " removed from the game");
	return true; //hm.
}

int Game::GetAmountOfPlayers() {
	return vPlayers.size();
}

void Game::CheckWinConditions() {
	if (iType == GAME_OVER || iType == GAME_LOBBY) return;
	if (vPlayers.size() < 1) return; //how?
	if (vPlayers.size() == 1) Win(vPlayers[0]); //shouldn't hit.
	for (size_t i = 0; i < vPlayers.size(); i++) {
		if (vPlayers[i]->GetCardAmount() < 1) Win(vPlayers[i]);
	}
}

void Game::Win(Player* player) {
	if (!player) return;
	Success("[G#" << GetId() << "] Game finished. Player #" << player->GetId() << " has won the game!");
	iType = GAME_OVER;
	iPlayerWonId = player->GetId();
}

bool Game::IsGameOver() {
	CheckWinConditions();
	if (iType == GAME_OVER) return true;
	else return false;
}

std::string Game::SendGameOverMessage() {
	if (!IsGameOver()) return std::string(" ");
	std::string out = "endgame|";
	out.append(std::to_string(iPlayerWonId));
	out.append("-");
	return out;
}

Card* Game::GetCardOnTop() {
	return pCardOnTop;
}



// executes the move (or doesn't, if not needed)
std::string Game::ExecuteMove(std::string datain) {
	if (IsGameOver()) return SendGameOverMessage();
	Msg("[G#" << GetId() << "] Executing move...");
	if (datain.rfind("first", 0) == 0) { //first round
		Msg("[G#" << GetId() << "] This is a first move!");
		iPlayerTurnId = vPlayers[0]->GetId(); // the first player begins
		std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds> (
			std::chrono::system_clock::now().time_since_epoch()
			);
		iTurnEndTime = s.count() + TURN_TIME;
		Success("[G#" << GetId() << "] Move execution finished!");
		return "AFS";
	}

	// playing the card
	if (datain.rfind("playcard|", 0) == 0) {
		Msg("[G#" << GetId() << "] Playing card...");
		std::string s = datain;
		int sap = s.find("|");
		s = s.substr(sap + 1, s.length());

		sap = s.find("|");
		std::string stringid = s.substr(0, sap);
		int playerid;
		try { playerid = stoi(stringid); }
		catch (const std::invalid_argument&) { return "AFS"; }
		catch (const std::out_of_range&) { return "AFS"; }
		catch (const std::exception&) { return "AFS"; }

		Player* pPlayer = GetSessionHandler()->GetPlayer(playerid);
		if (!pPlayer) {
			Warn("[G#" << GetId() << "] Can't finish move: unknown player");
			return "AFS";
		}

		if (iPlayerTurnId != pPlayer->GetId()) {
			Warn("[G#" << GetId() << "] Can't finish move: it's not player #" << pPlayer->GetId() << "s move!");
			return "AFS";
		}
		
		if (!IsPlayerInGame(pPlayer)) {
			Warn("[G#" << GetId() << "] Can't finish move: player not in game");
			return "AFS";
		}
		
		s = s.substr(sap + 1, s.length());
		sap = s.find("|");
		std::string card = s.substr(0, sap);
		
		Card* c = pPlayer->GetCard(stoi(s)-1);
		if (!c) {
			Warn("[G#" << GetId() << "] Can't finish move: player tried to play unknown card");
			return "AFS";
		}

		if (!Validate(c)) {
			Warn("[G#" << GetId() << "] Can't finish move: player tried to play mismatched card");
			return "AFS";
		}

		//reset cards function
		if (GetCardOnTop()->GetType() == TYPE_J || GetCardOnTop()->GetType() == TYPE_A) {
			iJNum = -1; iANum = -1;
		}

		PutOnTop(c);
		if (!pPlayer->RemoveCard(c)) {
			Warn("[G#" << GetId() << "] Can't finish move: Player doesn't have the card");
			return "AFS";
		}

		s = s.substr(sap + 1, s.length());
		sap = s.find("|");
		std::string func = s.substr(0, sap);

		iJNum = -1; iANum = -1;
		if (c->GetType() == TYPE_J) {
			iJNum = stoi(func);
		}
		else if (c->GetType() == TYPE_A) {
			iANum = stoi(func);
		}

		if (
			c->GetType() == TYPE_2 ||
			c->GetType() == TYPE_3 ||
			(c->GetType() == TYPE_K && c->GetSuit() == SUIT_SPADE) ||
			(c->GetType() == TYPE_K && c->GetSuit() == SUIT_HEART)
			) {
				switch (c->GetType()) {
				// by default, we pull 1 card, so we need to add to this number.
				// that's why a 2 card adds only 1 to the pull amount
				case TYPE_2: iToPull += 1; break; 
				case TYPE_3: iToPull += 2; break;
				case TYPE_K: iToPull += 4; break;
				default: break;
				}
		}


		std::chrono::seconds s2 = std::chrono::duration_cast<std::chrono::seconds> (
			std::chrono::system_clock::now().time_since_epoch()
			);
		iTurnEndTime = s2.count() + TURN_TIME;

		PassTurn();
		Success("[G#" << GetId() << "] Move execution finished!");
		return MsgGetGameStatus(pPlayer->GetId());
	}

	if (datain.rfind("drawcard|", 0) == 0) {
		Msg("[G#" << GetId() << "] Drawing a card...");
		std::string s = datain;
		int sap = s.find("|");
		s = s.substr(sap + 1, s.length());

		sap = s.find("|");
		std::string stringid = s.substr(0, sap);
		int playerid;
		try { playerid = stoi(stringid); }
		catch (const std::invalid_argument&) { return "AFS"; }
		catch (const std::out_of_range&) { return "AFS"; }
		catch (const std::exception&) { return "AFS"; }

		Player* pPlayer = GetSessionHandler()->GetPlayer(playerid);
		if (!pPlayer) {
			Warn("[G#" << GetId() << "] Can't finish move: unknown player");
			return "AFS";
		}

		if (pPlayer->GetId() != iPlayerTurnId) {
			Warn("[G#" << GetId() << "] Can't finish move: it's not player #" << pPlayer->GetId() << "s move!");
			return "AFS";
		}

		for (int i = 0; i < iToPull; i++) {
			Card* card = vDeck[0];
			if (!card) {
				Error("[G#" << GetId() << "] Unhandled Exception!");
				return "AFS";
			}

			TransferCardToPlayer(card, pPlayer);
		}
		iToPull = 1;

		std::chrono::seconds s3 = std::chrono::duration_cast<std::chrono::seconds> (
			std::chrono::system_clock::now().time_since_epoch()
			);
		iTurnEndTime = s3.count() + TURN_TIME;

		PassTurn();
		Success("[G#" << GetId() << "] Move execution finished!");
		return MsgGetGameStatus(pPlayer->GetId());
	}



	Success("[G#" << GetId() << "] Move execution finished!");
	return "AFS";
}

bool Game::Validate(Card* card) {
	// War:
	if (iToPull > 1) {
		if (card->GetType() == TYPE_2 || card->GetType() == TYPE_3 ||
			(card->GetType() == TYPE_K && card->GetSuit() == SUIT_SPADE) ||
			(card->GetType() == TYPE_K && card->GetSuit() == SUIT_HEART)
			) return true;
	}
	// A demands:
	else if (iANum > 0) {
		if (iANum == 1) { if (card->GetSuit() == SUIT_CLUB) return true; }
		else if (iANum == 2) { if (card->GetSuit() == SUIT_SPADE) return true; }
		else if (iANum == 3) { if (card->GetSuit() == SUIT_DIAMOND) return true; }
		else if (iANum == 4) { if (card->GetSuit() == SUIT_HEART) return true; }
	}
	// J demands:
	else if (iJNum > 0) {
		if (iJNum == 10) { if (card->GetType() == TYPE_10 || card->GetType() == TYPE_J) return true; }
		else if (iJNum == 2) { if (card->GetType() == TYPE_2 || card->GetType() == TYPE_J) return true; }
		else if (iJNum == 3) { if (card->GetType() == TYPE_3 || card->GetType() == TYPE_J) return true; }
		else if (iJNum == 4) { if (card->GetType() == TYPE_4 || card->GetType() == TYPE_J) return true; }
		else if (iJNum == 5) { if (card->GetType() == TYPE_5 || card->GetType() == TYPE_J) return true; }
		else if (iJNum == 6) { if (card->GetType() == TYPE_6 || card->GetType() == TYPE_J) return true; }
		else if (iJNum == 7) { if (card->GetType() == TYPE_7 || card->GetType() == TYPE_J) return true; }
		else if (iJNum == 8) { if (card->GetType() == TYPE_8 || card->GetType() == TYPE_J) return true; }
		else if (iJNum == 9) { if (card->GetType() == TYPE_9 || card->GetType() == TYPE_J) return true; }
	}
	// standard parsing:
	else if (
		card->GetSuit() == pCardOnTop->GetSuit()
		||
		card->GetType() == pCardOnTop->GetType()
		) return true;

	return false;
}


void Game::PassTurn() {
	int iTurn = -1;
	for (size_t i = 0; i < vPlayers.size(); i++) {
		if (vPlayers[i]->GetId() == iPlayerTurnId) iTurn = i;
	}
	if (iTurn == -1) return;

	if (iTurn == vPlayers.size()-1) iPlayerTurnId = vPlayers[0]->GetId();
	else iPlayerTurnId = vPlayers[iTurn + 1]->GetId();

	return;
}



// outputs the lobby status in the following syntax:
// lobbystatus|...|...|*lobbyid-
// ... - players in lobby
// 
// eg. lobbystatus|4|7|12|*4-
// which means: players with ID 4, 7, 12; in lobby ID 4
//
std::string Game::MsgGetLobbyStatus() {
	if (IsGameOver()) return SendGameOverMessage();
	std::string r = "lobbystatus|";
	for (size_t i = 0; i < vPlayers.size(); i++) {
		r.append(std::to_string(vPlayers[i]->GetId()));
		if (vPlayers[i]->GetId() == iGameHostId) r.append("H");
		r.append("|");
	}

	r.append("*");
	r.append(std::to_string(GetId()));
	r.append("-");

	return r;
}

std::string Game::MsgGetGameStatus(int playerid) {
	if (IsGameOver()) return SendGameOverMessage();
	std::string out = "gamestatus|";
	Player* p = GetSessionHandler()->GetPlayer(playerid);
	Card* cot = GetCardOnTop();

	std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds> (
		std::chrono::system_clock::now().time_since_epoch()
		);
	long long int currentTime = s.count();
	long long int remainingTime = iTurnEndTime - currentTime;
	if (remainingTime < 1) {
		iTurnEndTime = s.count() + TURN_TIME;

		// penalty: deal player with n cards
		for (int i = 0; i < iToPull; i++) {
			Card* card = vDeck[0];
			if (!card) {
				Error("[G#" << GetId() << "] Unhandled Exception!");
				return "AFS";
			}

			Player* player = GetSessionHandler()->GetPlayer(iPlayerTurnId);
			if (!player) {
				Error("[G#" << GetId() << "] Unhandled Exception!");
				return "AFS";
			}

			TransferCardToPlayer(card, player);
		}

		// reset the war counter
		iToPull = 1;

		PassTurn();
		return MsgGetGameStatus(playerid);
	}

	// playerlist
	for (size_t i = 0; i < vPlayers.size(); i++) {
		out.append(std::to_string(vPlayers[i]->GetId()));
		if (vPlayers[i]->GetId() == iGameHostId) out.append("H");
		if (vPlayers[i]->GetId() == iPlayerTurnId) out.append("T");
		if (vPlayers[i]->GetCardAmount() == 1) out.append("M");
		out.append("|");
	}
	out.append("*");

	//gameid
	out.append(std::to_string(GetId()));
	out.append("|");

	//iturn
	out.append(std::to_string(iTurn));
	out.append("|");

	//time
	out.append(std::to_string(remainingTime));
	out.append("|");

	//cards
	//todo: if(p)
	out.append(p->GetCards());

	//current card
	//todo if(cot)
	out.append(cot->GetString());
	out.append("|");

	//card function
	if (iANum == -1 && iJNum == -1) out.append("X");
	else if (iANum > 0) out.append(std::to_string(iANum));
	else out.append(std::to_string(iJNum));
	out.append("|");

	//cards to deal (if there's a war going on)
	out.append(std::to_string(iToPull));
	out.append("-"); //wip

	return out;
	//return "gamestatus|1|2|3|*10|32|43|CA|S0|DK|C4|*HQ|X|0-";
}

void Game::AddDeck() {
	Msg("[G#" << GetId() << "] Generating a new card deck...");
	int c = 0;
	for (size_t type = 0; type < TYPE_LENGTH; type++) {
		for (size_t suit = 0; suit < SUIT_LENGTH; suit++) {
			// make a new card
			vCards.push_back(new Card(static_cast<CARD_TYPE>(type), static_cast<CARD_SUIT>(suit)));
			
			// and push it to the deck
			vDeck.push_back(vCards[c]);
			c++;
		}
	}
}

void Game::Deal() {
	int c = 0;
	for (size_t i = 0; i < vPlayers.size(); i++) {
		Msg("[G#" << GetId() << "] Dealing player #" << vPlayers[i]->GetId() << " with 5 cards");
		for (size_t s = 0; s < 5; s++){
			TransferCardToPlayer(vDeck[0], vPlayers[i]);
		}
	}
}

void Game::TransferCardToPlayer(Card* card, Player* player) {
	if (!player) return;
	if (!card) return;

	player->AddCard(card);

	std::vector<Card*>::iterator pos = std::find(vDeck.begin(), vDeck.end(), card);
	if (pos != vDeck.end()) {
		vDeck.erase(pos);
	}
	else {
		// if this happens, something's horribly wrong!!
		// abort();
		return;
	}
}

Card* Game::FindCardToPutOnTop() {
	for (size_t i = 0; i < vDeck.size(); i++) {
		if (vDeck[i]->CanBePutOnTop()) return vDeck[i];
	}

	// if we didn't find the card to put on top,
	// then we add one deck and try once again
	AddDeck();
	return FindCardToPutOnTop();
}

void Game::PutOnTop(Card* card) {
	Msg("[G#" << GetId() << "] Putting a card on top");
	if (pCardOnTop) {
		vDeck.push_back(pCardOnTop);
	}
	pCardOnTop = card;
}

void Game::ExecuteFirstMove() {
	ExecuteMove("first");
}

Game::~Game() {

}