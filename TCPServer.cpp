/*
 * Driver.cpp
 *
 *  Created on: 11 июн. 2017 г.
 *	  Author: diman-pro
 */

#include "TCPServer.h"
#include "TCPClient.h"




namespace DP{
	namespace NET{
		namespace Server{
			bool Server::portIsAllow(const String & ip, size_t port) {
				#ifdef DP_WINDOWS
					WSAData wData;
					if (WSAStartup(MAKEWORD(2, 2), &wData) != 0){
						( log << "ERROR: " << "Fail to init WSA: " ) * WSAGetLastError();
						throw Error("Fail to init WSA");
					}
				#endif

				struct sockaddr_in serverAddress;
				lisen= socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				int turnOn = 1;
				#ifdef DP_WINDOWS
					if (setsockopt(lisen, SOL_SOCKET, SO_REUSEADDR, (char *) &turnOn, sizeof(turnOn)) == SOCKET_ERROR) {
						(log << "ERR: " ) * WSAGetLastError();
				#else
					if (setsockopt(lisen, SOL_SOCKET, SO_REUSEADDR, &turnOn, sizeof(turnOn)) == -1) {
				#endif
					#ifdef DEBUG
						perror("Set Socket Fail");
					#endif
					( log << "ERROR: ") * "Set Socket Fail";
					throw Error("Set Socket Fail");
				}
				serverAddress.sin_family = AF_INET;
				serverAddress.sin_addr.s_addr = inet_addr(ip.c_str());
				serverAddress.sin_port = htons(port);
				if (bind( lisen, (sockaddr *) &serverAddress, sizeof(serverAddress)) == -1){
					return false;
				}
				if (listen(lisen, 1000) == -1) {
					return false;
				}
				#ifdef DP_WINDOWS
					closesocket(lisen);
					WSACleanup();
				#else
					close(lisen);
				#endif
				return true;
			}

			void Server::Send(int client, const String& ans){
				while (send( client,  &ans[0], ans.size(), 0 ) != ans.size());
			}
			void Server::Send(int client, const char * ans, unsigned int length) {
				while (send(client, ans, length, 0) != length);
			}
			String Server::Read(int Client, char del){
				int err=10;
				char Buf=0;
				String query;
				while (err > 0 && Buf != del){
					if (Buf!=0)
						query+=Buf;
					err = recv(Client, &Buf, 1, 0);

				}
				if (err < 0)
					throw Error("recv failed");
				if (err == 0) //return ""; //клиент разорвал соединение
					throw Error("client destroy connect");
//				recv(Client, &Buf, 1, 0); // получаем \n
				#ifdef DEBUG
					//std::cout << "Data received2: " << query << "\n";
				#endif
				return query;
			}
			void Server::CloseClient(int C){
				close(C);
			}
			void Server::exit(){
				isRun=false;
				TCPClient cl;
				cl.Connect(listenHost, listenPort);
				cl.Close();
			}

			char * Server::ReadN(int Client, unsigned int count) {
				char * res = new char[count + 1];
				int err = recv(Client, res, count, 0);
				res[count] = 0;
				if (err <= 0) {
					delete[] res;
					return nullptr;
				}
				#ifdef DEBUG
					//std::cout << "Data received1: " << res << "\n";
				#endif
				return res;
			}

			//ToDo

			void Server::ThreadListen(String IP, size_t port, std::function<void(int)> proc){
				listenHost = IP;
				listenPort = port;

				#ifdef DP_WINDOWS
					WSAData wData;
					if (WSAStartup(MAKEWORD(2, 2), &wData) != 0){
						( log << "ERROR: " << "Fail to init WSA: " ) * WSAGetLastError();
						throw Error("Fail to init WSA");
					}
				#endif

				isRun=true;
				struct sockaddr_in serverAddress;
				lisen= socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				int turnOn = 1;
				#ifdef DP_WINDOWS
					if (setsockopt(lisen, SOL_SOCKET, SO_REUSEADDR, (char *) &turnOn, sizeof(turnOn)) == SOCKET_ERROR) {
						(log << "ERR: " ) * WSAGetLastError();
				#else
					if (setsockopt(lisen, SOL_SOCKET, SO_REUSEADDR, &turnOn, sizeof(turnOn)) == -1) {
				#endif
					#ifdef DEBUG
						perror("Set Socket Fail");
					#endif
					( log << "ERROR: ") * "Set Socket Fail";
					throw Error("Set Socket Fail");
				}
				serverAddress.sin_family = AF_INET;
				serverAddress.sin_addr.s_addr = inet_addr(IP.c_str());
				serverAddress.sin_port = htons(port);
				int errn=bind( lisen, (sockaddr *) &serverAddress, sizeof(serverAddress));
				if (errn == -1){
					#ifdef DEBUG
						perror("Bind failed:");
					#endif
					( log << "ERROR: ") * "Bind failed";
					throw Error("Bind failed:");
				}
				if (listen(lisen, 1000) == -1) {
					#ifdef DEBUG
						perror("Listen failed:");
					#endif
					( log << "ERROR: ") * "listen failed";
					throw Error("listen failed:");
				}
				Vector data;
				while(isRun){
					int *clientSocket = new int[1];
					*clientSocket = accept(lisen, NULL, NULL);
					if (!isRun)
						break;
					if (*clientSocket < 0) {
						__DP_LIB_NAMESPACE__::log << "accept failed" << *clientSocket << "\n";
						delete clientSocket;
					}
					Thread * t =new Thread(proc,*clientSocket);
					data.push_back(t);
					delete clientSocket;
				}
				for (int i=0;i<data.size();i++){
					data[i]->join();
				}

				#ifdef DP_WINDOWS
					WSACleanup();
				#endif
			}

			void Server::Listen(String IP, size_t port, std::function<void(int)> proc){
				listenHost = IP;
				listenPort = port;

				#ifdef DP_WINDOWS
					WSAData wData;
					if (WSAStartup(MAKEWORD(2, 2), &wData) != 0){
						( log << "ERROR: " << "Fail to init WSA: " ) * WSAGetLastError();
						throw Error("Fail to init WSA");
					}
				#endif

				isRun=true;
				struct sockaddr_in serverAddress;
				lisen= socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				int turnOn = 1;
				#ifdef DP_WINDOWS
					if (setsockopt(lisen, SOL_SOCKET, SO_REUSEADDR, (char *) &turnOn, sizeof(turnOn)) == SOCKET_ERROR) {
						(log << "ERR: " ) * WSAGetLastError();
				#else
					if (setsockopt(lisen, SOL_SOCKET, SO_REUSEADDR, &turnOn, sizeof(turnOn)) == -1) {
				#endif
					#ifdef DEBUG
						perror("Set Socket Fail");
					#endif
					( log << "ERROR: ") * "Set Socket Fail";
					throw Error("Set Socket Fail");
				}
				serverAddress.sin_family = AF_INET;
				serverAddress.sin_addr.s_addr = inet_addr(IP.c_str());
				serverAddress.sin_port = htons(port);
				if (bind( lisen, (sockaddr *) &serverAddress, sizeof(serverAddress)) == -1){
					#ifdef DEBUG
						perror("Bind failed:");
					#endif
					( log << "ERROR: ") * "Bind failed";
					throw Error("bind failed");
				}
				if (listen(lisen, 1000) == -1) {
					#ifdef DEBUG
						perror("Listen failed:");
					#endif
					( log << "ERROR: ") * "listen failed";
					throw Error("listen failed:");
				}
				while(isRun){
					int *clientSocket = new int[1];
					*clientSocket = accept(lisen, NULL, NULL);
					if (*clientSocket < 0)
						throw Error("accept failed");
					proc(*clientSocket);
				}

				#ifdef DP_WINDOWS
					closesocket(lisen);
					WSACleanup();
				#else
					close(lisen);
				#endif
			}
		}


	}
}

