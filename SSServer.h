#pragma once
#include "TCPServer.h"
#include <DPLib.conf.h>
#include <Types/Exception.h>
#include <algorithm>
#include <mutex>
#include <thread>
#include "ConsoleLoop.h"

using __DP_LIB_NAMESPACE__::String;

namespace std {
	//void getline()
}

struct SSStream {
	private:
		std::function<void(char *, int)> _sendf;
		std::function<char *(int)> _readf;
		std::function<void()> _closef;

		std::mutex * _bufferRead_locker = new std::mutex();
		__DP_LIB_NAMESPACE__::IStrStream * bufferRead = new __DP_LIB_NAMESPACE__::IStrStream();
		int bufferReadsize = 0;
		std::thread * read_thread;

		std::mutex * _bufferSend_locker = new std::mutex();
		__DP_LIB_NAMESPACE__::OStrStream * bufferSend = new __DP_LIB_NAMESPACE__::OStrStream();
		std::thread * send_thread;

		enum class CurMode{None, Send, Read};
		CurMode _mode = CurMode::Read;

		bool is_exit = false;
	public:
		SSStream(std::function<void(char *, int)> sendf, std::function<char *(int)> readf, std::function<void()> _closef): _sendf(sendf), _readf(readf), _closef(_closef) {
			read_thread = new std::thread(std::bind(&SSStream::ThreadRead, this));
			send_thread = new std::thread(std::bind(&SSStream::ThreadSend, this));
		}
		void close() {
			is_exit = true;
			_closef();
			read_thread->join();
			send_thread->join();
			delete read_thread;
			delete send_thread;
		}
		inline void flush() {}
		int size() {
			return bufferReadsize;
		}
		inline bool eof() const {
			return (is_exit == true) || (bufferSend==nullptr) || (bufferRead == nullptr);
		}
		template<typename T>
		inline friend SSStream & operator<<(SSStream & stream, const T & val) {
			stream._bufferSend_locker->lock();
			(*stream.bufferSend) << val;
			stream._bufferSend_locker->unlock();
			return stream;
		}
		template<typename T>
		inline friend SSStream & operator>>(SSStream & stream, T & val) {
			stream._bufferRead_locker->lock();
			(*stream.bufferRead) >> val;
			throw EXCEPTION("EX");
			stream.bufferReadsize -= stream.bufferRead->gcount();
			stream._bufferRead_locker->unlock();
			return stream;
		}
		inline String getline() {
			_bufferRead_locker->lock();
			String res;
			//std::cout << bufferRead->str();
			std::getline((*bufferRead), res);
			bufferReadsize -= res.size();
			bufferReadsize -= 1;
			_bufferRead_locker->unlock();
			return res;
		}

		void ThreadSend() {
			while (true) {
				if (bufferSend->str().size() > 0) {
					_bufferSend_locker->lock();
					String val = bufferSend->str();
					bufferSend->str("");
					if (val.size() > 256) {
						bufferSend->str(val.substr(255));
						val = val.substr(0, 254);
					}
					_bufferSend_locker->unlock();

					char * send = new char[val.size() + 1 + 1];
					send [0] = val.size();
					copy(val.begin(), val.end(), &(send[1]));
					for (int i =0; i < val.size() + 2; i++) {
						__DP_LIB_NAMESPACE__::log << (i==0 ? "" : " ") << (int)val[i];
					}
					__DP_LIB_NAMESPACE__::log << "\n";
					_sendf(send, val.size() + 1);
					delete [] send;
				}
				if (is_exit)
					break;
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
		}
		void ThreadRead() {
			while (true) {
				if (is_exit)
					break;

				unsigned char * data = (unsigned char* ) _readf(1);
				if (data == nullptr) {
					is_exit = true;
					break;
				}
				__DP_LIB_NAMESPACE__::log << "Readed: " << *data << ": ";
				int size = *data;
				char * readed = _readf(size);
				String tmp = readed;
				__DP_LIB_NAMESPACE__::log << tmp << "\n";

				_bufferRead_locker->lock();
				tmp = (bufferRead->eof()  ?  "" : bufferRead->str().substr( bufferRead->tellg() )) + tmp;
				bufferRead->str(tmp);
				bufferRead->seekg(0);
				bufferReadsize = tmp.size();
				_bufferRead_locker->unlock();

				delete [] readed;
				delete [] data;
			}
		}
};

void getline(SSStream & str, String & res);

class ShadowSocksServer{
	private:
		DP::NET::Server::Server *  _server;
		__DP_LIB_NAMESPACE__::List<ConsoleLooper<SSStream,SSStream>> loppers;



	public:
		void Start(int port = 8898);
		void StartInNewThread(int port = 8898);
		void Stop();



		void onNewClient(int id);
};
