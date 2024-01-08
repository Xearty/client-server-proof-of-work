#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <utility>
#include <chrono>
#include <miner.h>
#include <net_common.h>
#include <utils.h>
#include <requests.h>

#pragma comment(lib, "ws2_32.lib")

 bool setup_server(SOCKET& server_socket, unsigned short port) {
	 server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	 if (server_socket == INVALID_SOCKET) {
		 fputs("Could not create socket!", stderr);
		 return false;
	 }

	 sockaddr_in addr = {};
	 addr.sin_family = AF_INET;
	 addr.sin_port = htons(port);
	 inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

	 if (bind(server_socket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		 fprintf(stderr, "Socket bind failed with error code %d\n", WSAGetLastError());
		 return false;
	 }

	 if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
		 fprintf(stderr, "Socket could not listen: code %d\n", WSAGetLastError());
		 return false;
	 }

	 printf("Listening on port %d\n", port);
	 return true;
}

MineBlockResponse handle_mine_block_request(MineBlockRequest req) {
	MinedBlock mined_block = mine_block_parallel(req.block, req.difficulty, req.thread_count);
	MineBlockResponse res = {
		mined_block.block,
		mined_block.hash,
	};
	return res;
}

 void handle_requests(SOCKET server_socket) {
	 while (true) {
		SOCKET client_socket = accept(server_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
			fprintf(stderr, "Accept failed with error: %d\n", WSAGetLastError());
			break;
		}

		puts("Accepted connection");

		Request req;
		recv(client_socket, (char*)&req, sizeof(req), MSG_PEEK);

		switch (req.type) {
			case RequestType::MineBlock: {
				MineBlockRequest mine_block_req;
				recv(client_socket, (char*)&mine_block_req, sizeof(MineBlockRequest), MSG_WAITALL);

				std::thread response_thread(RequestHandler(std::function(handle_mine_block_request)), client_socket, mine_block_req);
				response_thread.detach();
			} break;

			case RequestType::MineBlockTimed: {
				MineBlockTimedRequest mine_block_timed_req;
				recv(client_socket, (char*)&mine_block_timed_req, sizeof(MineBlockTimedRequest), MSG_WAITALL);

				std::function handler = [](MineBlockTimedRequest mine_block_timed_req) {
					auto before = std::chrono::steady_clock::now();
					MineBlockResponse res = handle_mine_block_request(mine_block_timed_req);

					auto after = std::chrono::steady_clock::now();
					auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();

					MineBlockTimedResponse timed_res;
					memcpy(&timed_res, &res, sizeof(res));
					timed_res.milliseconds_taken = delta;
					return timed_res;
				};

				std::thread response_thread(RequestHandler(handler), client_socket, mine_block_timed_req);
				response_thread.detach();
			} break;
		}
	 }
 }

int main() {
	if (!initialize_websocket_api()) {
		return 1;
	}
	defer(WSACleanup());

	SOCKET server_socket;
	if (!setup_server(server_socket, 5000)) {
		if (server_socket != INVALID_SOCKET) {
			close_socket(server_socket);
		}
		return 1;
	}

	defer(close_socket(server_socket));

	handle_requests(server_socket);
	puts("Stopping server");
	system("pause");
}
