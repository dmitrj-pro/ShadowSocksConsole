#pragma once
#include <Network/TCPClient.h>
#include <mutex>
#include <_Driver/ThreadWorker.h>
#include <Types/Exception.h>
#include <iostream>
#include "SSServer.h"


class ShadowSocksRemoteClient{
	private:
		__DP_LIB_NAMESPACE__::Thread * th;
		__DP_LIB_NAMESPACE__::TCPClient client;
		SSStream * stream;
		bool is_exit = false;
	public:
		void Start(const String & host, unsigned short port = 8898) {
			client.Connect(host, port);
			stream = new SSStream([this](char * text, int count) {
				client.Send(text, count);
			}, [this](int count) {
				return client.ReadN(count);
			}, [this]() {
				client.Close();
			});
			th = new __DP_LIB_NAMESPACE__::Thread(&ShadowSocksRemoteClient::ThreadRead, this);
			th->SetName("ShadowSocksRemoteClient::ThreadRead");
			th->start();
			while (!is_exit) {
				String cmd;
				getline(std::cin, cmd);
				if (cmd == "exit" || cmd == "quit") {
					(*stream) << "disconnect\n";
					is_exit = true;
					th->join();
					break;
				}
				if (cmd == "EXIT") {
					(*stream) << "exit\n";
					is_exit = true;
					th->join();
					break;
				}


				(*stream) << cmd << "\n";
			}
		}
		void ThreadRead() {
			while (true) {
				if (is_exit)
					break;

				if (stream->size() > 0) {
					String line = stream->getline();
					std::cout << line << "\n";
					std::cout.flush();
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
		}
};
