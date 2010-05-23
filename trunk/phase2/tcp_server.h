#include <sys/types.h>

class TCPServer {
	int _listener;
	int _client;

	int _port;

public:
	TCPServer(int port = 1234);
	~TCPServer();

	int update();

	void send_ball_pos(float x, float y, char prefix);
	void send_theta(double theta);
	void send_raw(char *str);
	void send_refetch();
};
