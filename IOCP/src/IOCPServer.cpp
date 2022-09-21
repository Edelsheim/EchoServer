#include "IOCPServer.hpp"

IOCPServer::IOCPServer(const u_short& port)
{
	this->iocp = NULL;
	this->listenSocket = INVALID_SOCKET;
	this->serverPort = port;

	// set server address
	ZeroMemory(&this->serverAddr, sizeof(SOCKADDR_IN));
	this->serverAddr.sin_family = AF_INET;
	this->serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	this->serverAddr.sin_port = htons(this->serverPort);

	// vector clear
	this->completionThreads.clear();

	// session clear
	this->session = std::make_unique<concurrency::concurrent_unordered_map<ULONG_PTR, SOCKET>>();
}

IOCPServer::~IOCPServer()
{
	delete this->session.release();
	::WSACleanup();
}

bool IOCPServer::Run()
{
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
		std::cout << "WSA startup fail" << std::endl;
		std::cout << ::GetLastError() << std::endl;
		return false;
	}

	// make iocp
	this->iocp = this->CreateIOCP();
	if (this->iocp == NULL) {
		std::cout << "Create iocp fail" << std::endl;
		std::cout << ::GetLastError() << std::endl;
		return false;
	}

	// make listen socket
	if (!this->MakeSocket(this->listenSocket)) {
		std::cout << "make socket fail" << std::endl;
		std::cout << ::GetLastError() << std::endl;
		return false;
	}

	// bind socket
	if (!this->BindSocket(this->listenSocket)) {
		std::cout << "bind socket fail" << std::endl;
		std::cout << ::GetLastError() << std::endl;
		::closesocket(this->listenSocket);
		return false;
	}

	// listen socket
	if (!this->ListenSocket(this->listenSocket, SOMAXCONN)) {
		std::cout << "Listen socket fail" << std::endl;
		std::cout << ::GetLastError() << std::endl;
		::closesocket(this->listenSocket);
		return false;
	}

	// create GetQueuedCompletionStatus thread
	this->MakeWorkingThreads();

	std::cout << "Server listen port : " << this->serverPort << std::endl;
	// accept
	while (true) {
		SOCKADDR_IN client_addr{};
		INT addr_len = sizeof(SOCKADDR_IN);

		std::cout << "Ready accept client" << std::endl;
		SOCKET client = ::WSAAccept(this->listenSocket, (sockaddr*)&client_addr, &addr_len, NULL, NULL);
		if (client == SOCKET_ERROR) {
			std::cout << "Accept client fail" << std::endl;
			std::cout << ::GetLastError() << std::endl;
			continue;
		}

		static size_t id = 0; // client id?????
		if (this->session->insert(std::make_pair(id, client)).second == true) {
			printf("Session insert id : %lld\n", id);
			if (this->WatchSocket(client, id) == false) {
				printf("Session watch fail\n");
				closesocket(client);
				continue;
			}
			id++;

			SOCKETOVERLAPPED socket_overlapped;
			ZeroMemory(&socket_overlapped, sizeof(SOCKETOVERLAPPED));
			socket_overlapped.buff.len = 2048;
			socket_overlapped.buff.buf = socket_overlapped.message;
			DWORD trans_bytes = 0;
			DWORD flag = 0;
			int a = ::WSARecv(client, &(socket_overlapped.buff), 1, &trans_bytes, &flag, &socket_overlapped, NULL);
			if (a == SOCKET_ERROR) {
				int last_error = WSAGetLastError();
				if (ERROR_IO_PENDING != last_error) {
					printf("Accept Recv error\n");
					closesocket(client);
				}
			}
		}
		else {
			std::cout << "Watch client " << id << " fail" << std::endl;
			std::cout << ::GetLastError() << std::endl;
		}
	}
	
	for (std::thread& th : this->completionThreads)
		th.join();

	return true;
}

HANDLE IOCPServer::CreateIOCP()
{
	return ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
}

void IOCPServer::MakeWorkingThreads()
{
	SYSTEM_INFO system_info;
	::GetSystemInfo(&system_info);
	DWORD num_of_thread = system_info.dwNumberOfProcessors * 2;

	for (DWORD i = 0; i != num_of_thread; i++)
		this->completionThreads.push_back(std::thread(&IOCPServer::CompletionThread, this));
}

bool IOCPServer::MakeSocket(SOCKET& socket)
{
	socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (socket == INVALID_SOCKET)
		return false;
	else
		return true;
}

bool IOCPServer::BindSocket(SOCKET& socket)
{
	if (socket == INVALID_SOCKET)
		return false;

	if (::bind(socket, reinterpret_cast<const sockaddr*>(&this->serverAddr), sizeof(this->serverAddr)) == SOCKET_ERROR)
		return false;
	else
		return true;
}

bool IOCPServer::ListenSocket(SOCKET& socket, const int& backlog)
{
	if (socket == INVALID_SOCKET)
		return false;

	if (::listen(socket, backlog) == SOCKET_ERROR)
		return false;
	else
		return true;
}

bool IOCPServer::SetSocketOpt(SOCKET& socket, const int& opt, const char* buff)
{
	if (::setsockopt(socket, SOL_SOCKET, opt, buff, sizeof(buff)))
		return false;
	else
		return true;
}

bool IOCPServer::WatchSocket(const SOCKET& socket, const ULONG_PTR& watchKey)
{
	HANDLE handle = CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), this->iocp, watchKey, 0);
	if (handle == this->iocp) {
		return true;
	}
	else {
		return false;
	}
}

DWORD __stdcall IOCPServer::CompletionThread()
{
	while (true) {
		DWORD bytes_transferred = 0;
		ULONG_PTR session_id = 0;
		SOCKETOVERLAPPED* overlapped;

		BOOL check = GetQueuedCompletionStatus(
			this->iocp,
			&bytes_transferred,
			&session_id,
			(LPOVERLAPPED*)(&overlapped),
			INFINITE
		);

		if (check == FALSE) {
			printf("%lld is status fail\n", session_id);
			closesocket(this->session->find(static_cast<int>(session_id))->second);
			this->session->find(static_cast<int>(session_id))->second = NULL;
			Sleep(1);
			continue;
		}
		else if (bytes_transferred == 0) {
			printf("%lld is transferred is 0\n", session_id);
			closesocket(this->session->find(static_cast<int>(session_id))->second);
			this->session->find(static_cast<int>(session_id))->second = NULL;
			Sleep(1);
			continue;
		}
		else {
			std::string data = std::string(overlapped->buff.buf);
			printf("(%lld) data : %s\n", session_id, data.c_str());

			SOCKET session_socket = this->session->find(static_cast<int>(session_id))->second;

			overlapped->buff.len = (ULONG)data.length();
			overlapped->buff.buf = (CHAR*)data.c_str();

			DWORD flag = 0;
			int send_check = ::WSASend(session_socket, &(overlapped->buff), 1, &flag, 0, NULL, NULL);

			ZeroMemory(overlapped, sizeof(SOCKETOVERLAPPED));
			ZeroMemory(overlapped->message, 2048);
			overlapped->buff.len = 2048;
			overlapped->buff.buf = overlapped->message;
			flag = 0;
			::WSARecv(session_socket, &overlapped->buff, 1, &bytes_transferred, &flag, overlapped, NULL);
		}
	}
	return 0;
}

