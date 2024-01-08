#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <optional>

#include <miner.h>
#include <net_common.h>
#include <utils.h>
#include <requests.h>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 5000;

template <typename RequestT>
std::optional<typename ResponseType<RequestT>::type> make_request(sockaddr_in addr, RequestT request) {
	SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		fputs("Could not create socket!", stderr);
		return std::nullopt;
	}

	defer(close_socket(sock));

	if (connect(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		fprintf(stderr, "Could not connect. Error code %d\n", WSAGetLastError());
		return std::nullopt;
	}
	
	if (send(sock, (char*)&request, sizeof(request), 0) != SOCKET_ERROR) {
		puts("Request sent");

		MineBlockTimedResponse response;
		if (recv(sock, (char*)&response, sizeof(response), 0) != SOCKET_ERROR) {
			puts("Response received");
			return response;
		} else {
			fputs("Response could not be received", stderr);
		}
	} else {
		fputs("Request could not be sent", stderr);
	}
	
	return std::nullopt;
}

int main() {
	if (!initialize_websocket_api()) {
		return 1;
	}
	defer(WSACleanup());

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
	
	size_t difficulty = 6;
	char message[] = "hello";
	Block block;
	block.previous_block_hash = sha256(message);

	{
		std::cout << "Single-threaded performance:" << std::endl;
		
		MineBlockTimedRequest req;
		req.block = block;
		req.difficulty = difficulty;
		req.thread_count = 1;

		std::optional<MineBlockTimedResponse> res = make_request(addr, req);

		if (res.has_value()) {
			std::cout << "hash: " << to_hex(res->hash) << std::endl;
			std::cout << "time taken: " << res->milliseconds_taken << "ms" << std::endl;
		}
	}

	{
		size_t thread_count = 6;
		std::cout << "Multi-threaded performance (" << thread_count << " threads):" << std::endl;
		
		MineBlockTimedRequest req;
		req.block = block;
		req.difficulty = difficulty;
		req.thread_count = thread_count;

		std::optional<MineBlockTimedResponse> res = make_request(addr, req);

		if (res.has_value()) {
			std::cout << "hash: " << to_hex(res->hash) << std::endl;
			std::cout << "time taken: " << res->milliseconds_taken << "ms" << std::endl;
		}
	}
	
	system("pause");
}
