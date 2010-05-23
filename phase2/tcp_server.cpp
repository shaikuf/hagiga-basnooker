#include "tcp_server.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <errno.h>

using namespace std;

TCPServer::TCPServer(int port) {
	// initialize the members
	_port = port;
	_client = 0;

	// initialize winsock

	WSADATA wsaData;   // if this doesn't work
	//WSAData wsaData; // then try this instead

		// MAKEWORD(1,1) for Winsock 1.1, MAKEWORD(2,0) for Winsock 2.0:

	if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed.\n");
		exit(1);
	}

	// initialize the server

		// get us a socket and bind it
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	int retval;

	ZeroMemory(&hints, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

		// Resolve the local address and port to be used by the server
	char port_str[128];
	_snprintf_s(port_str, 128, "%d", _port);
	retval = getaddrinfo(NULL, port_str, &hints, &result);
	if (retval != 0) {
		printf("getaddrinfo failed: %d\n", retval);
		WSACleanup();
		exit(1);
	}

		// create socket
	_listener = INVALID_SOCKET;
	_listener = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (_listener == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(1);
	}

		// bind socket
	retval = bind( _listener, result->ai_addr, (int)result->ai_addrlen);
	if (retval == SOCKET_ERROR) {
		printf("bind failed: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(_listener);
		WSACleanup();
		exit(1);
	}

	// listen on socket
	if ( listen( _listener, SOMAXCONN ) == SOCKET_ERROR ) {
		printf( "Error at bind(): %ld\n", WSAGetLastError() );
		closesocket(_listener);
		WSACleanup();
		exit(1);
	}

	// set socket to be non-blocking
	u_long arg = 1;
	ioctlsocket(_listener, FIONBIO, &arg);
}

TCPServer::~TCPServer() {
	closesocket(_listener);

	if(_client != 0)
		closesocket(_client);

	WSACleanup();
}

int TCPServer::update() {
	if(_client == 0) { // IF WE HAVE NO CLIENT
		// accept a client socket
		_client = INVALID_SOCKET;
		_client = accept(_listener, NULL, NULL);

		if (_client == INVALID_SOCKET) {
			if(WSAGetLastError() == WSAEWOULDBLOCK) {
				// pass
			} else {
				printf("accept failed: %d\n", WSAGetLastError());
			}

			_client = 0;
		}

		return 0;
	} else { // WE HAVE A CLIENT
		// try to receive
		char buf[128];
		int retval = recv(_client, buf, 128, 0);

		if(retval == 0) { // connection closed
			closesocket(_client);
			_client = 0;

			return 0;

		} else if(retval == -1) { // error
			if(WSAGetLastError() == WSAEWOULDBLOCK) {
				// pass
			} else if(WSAGetLastError() == 10054) { // closed
				_client = 0;
			} else {
				printf("Error at recv(): %ld\n", WSAGetLastError());
			}

			return 0;
		} else {
			// received something
			return 1;
		}
	}
}

void TCPServer::send_ball_pos(float x, float y, char prefix) {
	if(_client == 0)
		return;

	char buf[128];
	_snprintf_s(buf, 128, "%c%f,%f\n", prefix, x, y);
	if(send(_client, buf, strlen(buf), 0) == -1) {
		if(WSAGetLastError() == 10054) {
			_client = 0;
		} else {
			printf("Error at send(): %ld\n", WSAGetLastError());
			exit(1);
		}
	}
}

void TCPServer::send_refetch() {
	if(_client == 0)
		return;

	char buf[128];
	_snprintf_s(buf, 128, "refetch\n");
	if(send(_client, buf, strlen(buf), 0) == -1) {
		if(WSAGetLastError() == 10054) {
			_client = 0;
		} else {
			printf("Error at send(): %ld\n", WSAGetLastError());
			exit(1);
		}
	}
}

void TCPServer::send_theta(double theta) {
	if(_client == 0)
		return;

	char buf[128];
	_snprintf_s(buf, 128, "%f\n", theta);
	if(send(_client, buf, strlen(buf), 0) == -1) {
		if(WSAGetLastError() == 10054) {
			_client = 0;
		} else {
			printf("Error at send(): %ld\n", WSAGetLastError());
			exit(1);
		}
	}
}

void TCPServer::send_raw(char *str) {
	if(_client == 0)
		return;

	char buf[128];
	_snprintf_s(buf, 128, "%s", str);
	if(send(_client, buf, strlen(buf), 0) == -1) {
		if(WSAGetLastError() == 10054) {
			_client = 0;
		} else {
			printf("Error at send(): %ld\n", WSAGetLastError());
			exit(1);
		}
	}
}