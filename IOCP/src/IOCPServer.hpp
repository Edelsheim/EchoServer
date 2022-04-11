#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <atomic>
#include <vector>
#include <thread>

// session
#include <concurrent_unordered_map.h>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

struct SOCKETOVERLAPPED : public WSAOVERLAPPED {
	WSABUF buff;
};

class IOCPServer
{
public:
	IOCPServer(const u_short& port);
	~IOCPServer();

	bool Run();

private:
	unsigned short serverPort;

	HANDLE iocp;
	SOCKET listenSocket;
	SOCKADDR_IN serverAddr;
	std::vector<std::thread> completionThreads;

	std::unique_ptr<concurrency::concurrent_unordered_map<ULONG_PTR, SOCKET>> session;

	HANDLE CreateIOCP();

	void MakeWorkingThreads();

	bool MakeSocket(SOCKET& socket);

	bool BindSocket(SOCKET& socket);

	bool ListenSocket(SOCKET& socket, const int& backlog);

	bool SetSocketOpt(SOCKET& socket, const int& opt, const char* buff);

	bool WatchSocket(const SOCKET& socket, const ULONG_PTR& watchKey);

	// thread call
	DWORD __stdcall CompletionThread();
};