#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include <sys/types.h>
#include "params.h"

class TCPServer {
	int _listener;
	int _client;

	int _port;

public:
	TCPServer(int port = PORT);
	~TCPServer();

	// if we don't have a new connection, check for one. if we do --
	// check for incoming messages
	// returns 1 if we got a msg (only one is "refetch")
	int update();

	// Come on! these are trivial. I'm too tired to do this.
	void send_ball_pos(float x, float y, char prefix);
	void send_theta(double theta);
	void send_raw(char *str);
	void send_refetch();
};

#endif