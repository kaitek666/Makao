#include "SocketHandler.h"
#include "SessionHandler.h"
#include <ctime>
#include <string>
#include "Game.h"

using namespace std;

void Handle() {
	long SUCCESSFUL;
	WSAData WinSockData;
	WORD DLLVERSION;

	DLLVERSION = MAKEWORD(2, 1);
	SUCCESSFUL = WSAStartup(DLLVERSION, &WinSockData);

	SOCKADDR_IN ADDRESS;
	int AddressSize = sizeof(ADDRESS);

	SOCKET sock_LISTEN;
	SOCKET sock_CONNECTION;

	string CONVERTER;
	char message[200];

	sock_CONNECTION = socket(AF_INET, SOCK_STREAM, NULL);
	ADDRESS.sin_addr.S_un.S_addr = inet_addr("192.168.1.77");
	ADDRESS.sin_family = AF_INET;
	ADDRESS.sin_port = htons(444);

	sock_LISTEN = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sock_LISTEN, (SOCKADDR*)&ADDRESS, sizeof(ADDRESS));
	listen(sock_LISTEN, SOMAXCONN);

	while (true) {
		cout << "\n\tSERVER: Waiting for incoming connection...";

		if (sock_CONNECTION = accept(sock_LISTEN, (SOCKADDR*)&ADDRESS, &AddressSize)) {

			struct sockaddr_in* s = (struct sockaddr_in*)&ADDRESS;

			std::time_t t = std::time(0);

			// _CRT_SECURE_NO_WARNINGS
			std::tm* now = std::localtime(&t);

			std::string add0 = "";
			if (now->tm_sec + 1 < 10) add0 = "0";

			cout
				<< "\n\t"
				<< "[" << (now->tm_mday) << "-" << (now->tm_mon + 1) << "-" << (now->tm_year + 1900) << " "
				<< (now->tm_hour) << ":" << (now->tm_min) << ":" << add0 << (now->tm_sec + 1) << "] "
				<< "Incoming connection from " << inet_ntoa(s->sin_addr) << "..."
				<< endl;
			SUCCESSFUL = send(sock_CONNECTION, "Hi", 3, NULL); // dummy response to let the client know we are here.
			SUCCESSFUL = recv(sock_CONNECTION, message, sizeof(message), NULL); // get the message from the client
			CONVERTER = message;
			std::string st = message;
			cout << "\tMessage from CLIENT: \t" + CONVERTER;

			bool sendStatus = false;
			bool lobbyPlayer = false;

			std::string response = "Hi";
			//assign new ID
			if (st == "status") {
				response = std::to_string(GetSessionHandler()->CreatePlayer());
				response = "newid|" + response + "-";
			}

			//handle the status
			else if (st.rfind("status|", 0) == 0) {
				std::string tocut;
				tocut = st.substr(7, st.length());
				int playerid = stoi(tocut);

				Player* player = GetSessionHandler()->GetPlayer(playerid);

				// if in game...
				Game* g = GetSessionHandler()->IsPlayerInGame(player);
				if (g) {
					// in lobby:
					if (g->IsLobby()) {
						response = g->MsgGetLobbyStatus();
					}
					// in game:
					else {
						response = g->MsgGetGameStatus(playerid);
					}
				}
				//akcja
			}

			//create lobby
			if (st.rfind("createlobby|", 0) == 0) {
				lobbyPlayer = true;

				// get the player id from the string
				std::string tocut;
				tocut = st.substr(12, st.length());
				int playerid = stoi(tocut);

				int gameid;
				Game* game;
				Player* player = GetSessionHandler()->GetPlayer(playerid);

				// try to add the player to the room
				if (player) {
					gameid = GetSessionHandler()->CreateGame();
					game = GetSessionHandler()->GetGame(gameid);
					if (game) {
						if (game->AddPlayer(player)) { //returns true only on success
							game->SetGameHost(player);
							sendStatus = true;
						}
					}
				}

				// if above succeeded, send the status message
				if (sendStatus) {
					game = GetSessionHandler()->GetGame(gameid); //it's stupid to assign it again, but the compiler is moody today
					if (game) {
						game->MakeLobby();
						response = game->MsgGetLobbyStatus();
					}
				}
			}

			if (st.rfind("lobbylist|", 0) == 0) {
				response = GetSessionHandler()->GetGameList();
			}

			if (st.rfind("leavelobby|", 0) == 0) {
				std::string tocut;
				tocut = st.substr(11, st.length());
				int playerid = stoi(tocut);
				GetSessionHandler()->KickPlayer(playerid);
				response = "Hi"; //defaulting the response
			}

			if (st.rfind("joinlobby|", 0) == 0) {
				std::string tocut;
				tocut = st.substr(10, st.length());
				if (GetSessionHandler()->ParseAndExecuteJoinLobby(tocut)) {
					response = "AFS"; //Ask For Status
				}
				else {
					response = "Hi"; //defaulting the response
				}
			}


			if (st.rfind("begingame|", 0) == 0) {
				std::string tocut;
				tocut = st.substr(10, st.length());
				GetSessionHandler()->ParseAndExecuteBeginGame(tocut);
				response = "AFS";
			}


			SUCCESSFUL = send(sock_CONNECTION, response.c_str(), response.length(), NULL); // respond to the clients message
			cout << "\n\tMessage to CLIENT:   \t" << response << endl;

			
		}

	}
}