#include <sys/types.h>

class TCPServer {
	int _listener;
	int _client;

	int _port;

public:
	TCPServer(int port = 1234);
	~TCPServer();

	int update();

	void send_white_pos(float x, float y);
	void send_theta(double theta);
};
