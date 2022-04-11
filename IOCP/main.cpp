#pragma once

#include "src/IOCPServer.hpp"

int main(void) {
	std::unique_ptr<IOCPServer> server = std::make_unique<IOCPServer>(3501);

	server->Run();

	delete server.release();
	return 0;
}