#pragma once

#include <iostream>

#include "src/IOCPClient.hpp"

int main(void) {

	std::unique_ptr<IOCPClient> client = std::make_unique<IOCPClient>();
	if (client->Init()) {
		if (client->Connect("127.0.0.1", 3501)) {

			client->RecvData([](std::string data) {
				std::cout << "recv data : " << data << std::endl;
			});

			client->Send("hello");
		}
	}
	while (1);

	delete client.release();
	return 0;
}