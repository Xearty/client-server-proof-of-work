#pragma once

#include <functional>

#include <net_common.h>
#include <miner.h>

enum class RequestType {
	MineBlock,
	MineBlockTimed,
};

struct Request {
	RequestType type;
};

struct MineBlockRequest : Request {
	Block block;
	size_t difficulty;
	size_t thread_count;

	explicit MineBlockRequest() {
		type = RequestType::MineBlock;
	}
};

struct MineBlockResponse {
	Block block;
	Sha256 hash;
};

struct MineBlockTimedRequest : MineBlockRequest {
	explicit MineBlockTimedRequest() {
		type = RequestType::MineBlockTimed;
	}
};

struct MineBlockTimedResponse : MineBlockResponse {
	size_t milliseconds_taken;
};

template <typename RequestT>
struct ResponseType {};

template <> struct ResponseType<MineBlockRequest> {
	using type = MineBlockResponse;
};

template <> struct ResponseType<MineBlockTimedRequest> {
	using type = MineBlockTimedResponse;
};

template <typename RequestT>
struct RequestHandler {
	using HandlerType = std::function<typename ResponseType<RequestT>::type(RequestT)>;

	explicit RequestHandler(const HandlerType& handler) : handler(handler) {}
	explicit RequestHandler(HandlerType&& handler) : handler(std::move(handler)) {}

	void operator()(SOCKET client_socket, RequestT request) {
		typename ResponseType<RequestT>::type res = handler(request);
		send(client_socket, (char*)&res, sizeof(res), 0);
		close_socket(client_socket);
	}
private:
	HandlerType handler;
};
