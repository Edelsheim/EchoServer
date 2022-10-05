#pragma once

#include <iostream>
#include <atomic>
#include <thread>
#include <functional>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

class IOCPClient {
public:
	IOCPClient() {
		::closesocket(this->client);
		::WSACleanup();
		this->client = INVALID_SOCKET;
		this->recvThreadRun = false;
		this->recvThread = nullptr;
	}

	~IOCPClient() {
		this->recvThreadRun = false;

		if (this->recvThread != nullptr) {
			if (this->recvThread->joinable())
				this->recvThread->join();
			
			delete this->recvThread.release();
		}

		::closesocket(this->client);
		::WSACleanup();
	}

	const bool Init() {
		WSADATA wsa_data;
		if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
			std::cout << "WSA startup fail" << std::endl;
			std::cout << ::GetLastError() << std::endl;
			return false;
		}

		if (MakeSocket(this->client) == false) {
			std::cout << "Make client socket fail" << std::endl;
			std::cout << ::GetLastError() << std::endl;
			return false;
		}

		return true;
	}

	const bool Connect(const std::string ip, const u_short port) {
		if (this->client == INVALID_SOCKET)
			return false;

		SOCKADDR_IN server_addr;
		ZeroMemory(&server_addr, sizeof(SOCKADDR_IN));
		inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr.S_un.S_addr);
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);

		int a = ::connect(this->client, (const sockaddr*)&server_addr, sizeof(SOCKADDR_IN));
		printf("connect : %d\n", a);
		if (a >= 0) {
			this->recvThread = std::make_unique<std::thread>(&IOCPClient::RecvRun, this);
			return true;
		}
		else {
			return false;
		}
	}

	const bool Send(std::string str) {
		int a = ::send(this->client, str.c_str(), static_cast<int>(str.length()), 0);
		std::cout << "Send : " << str << std::endl;
		printf("send : %d\n", a);
		return (a > 0) ? true : false;
	}

	void RecvData(const std::function<void(std::string)>& handler) {
		if (handler != nullptr)
			this->handler = handler;
	}

private:
	SOCKET client;
	std::atomic_bool recvThreadRun;
	std::unique_ptr<std::thread> recvThread;
	std::function<void(std::string)> handler;

	const bool MakeSocket(SOCKET& socket)
	{
		socket = ::socket(AF_INET, SOCK_STREAM, 0);
		if (socket == INVALID_SOCKET)
			return false;
		else
			return true;
	}

	int RecvRun(void) {
		this->recvThreadRun = true;

		while (this->recvThreadRun) {
			char buf[2048] = {0, };
			int recv_result = ::recv(this->client, buf, 2048, 0);

			if (recv_result >= 0)
				this->handler(std::string(buf));
		}

		return 0;
	}
};