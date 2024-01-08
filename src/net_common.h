#pragma once

#define NOMINMAX
#include <winsock2.h>
#include <functional>
#include <stdio.h>

inline bool initialize_websocket_api() {
	WSADATA wsaData;
	if (int errcode = WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed with error: %d\n", errcode);
		return false;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		printf("Could not find a usable version of Winsock.dll. Version used %d.%d\n",
			HIBYTE(wsaData.wVersion), LOBYTE(wsaData.wVersion));
	}

	return true;
}

inline void close_socket(SOCKET sock) {
	if (closesocket(sock) != 0) {
		fprintf(stderr, "closesocket error: code %d\n", WSAGetLastError());
	}
}
