
/*
 * Driver.h
 *
 *  Created on: 17 мая 2017 г.
 *	  Author: diman-pro
 */

 #pragma once
 
 #ifdef DP_LINUX
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

#ifdef DP_WINDOWS
	#include <winsock2.h>
	#include <winsock.h>
	#include <unistd.h>
#endif


//#include <netinet/in.h>
//#include <netdb.h>
#include <Log/Log.h>

#include <thread>
#include <string>
#include <vector>
#include <functional>

#define DEBUG



#ifdef DEBUG
#include <iostream>
#include <stdio.h>
#endif

#include <Log/Log.h>

using __DP_LIB_NAMESPACE__::log;


namespace DP{
	namespace NET{
		namespace Server{
			typedef  std::string String;
			typedef std::thread Thread;
			typedef std::vector<Thread*> Vector;

			class Error{
				private:
					String str;
				public:
					Error(const String& str):str(str){}
					String about (){ return str; }
			};

			class Server{
				private:
					int lisen;
					String listenHost;
					int listenPort;
					bool isRun=true;
				public:
					inline Server():lisen(-1){}
					//Begin Listen IP:port with function proc. Server
					//create new thread for proc.
					void ThreadListen(String IP, size_t port, std::function<void(int)> proc);
					//Begin Listen IP:port with function proc. Server no
					//create new thread for proc.
					void Listen(String IP, size_t port, std::function<void(int)> proc);
					bool portIsAllow(const String & ip, size_t port);
					//Send user id 'client' data ans
					static void Send(int client, const String& ans);
					//Send user id 'client' data ans
					static void Send(int client, const char * ans, unsigned int length);
					//Read user question before input del
					static String Read(int Client,char del='\r');
					static char * ReadN(int Client, unsigned int count);
					//Close Client
					static void CloseClient(int C);
					//Close Server
					void exit();

			};


		}
	}
}

