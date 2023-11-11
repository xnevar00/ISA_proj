#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

#define IPADDRLEN 16
enum StatusCode {
    SUCCESS = 0,
    INVALID_ARGUMENTS = -1,
    SOCKET_ERROR = -2,
    CONNECTION_ERROR = -3,
    PACKET_ERROR = -4,
};

enum class ServerState {
    Idle,
    ReceiveWriteRequest,
    SendAcknowledgement,
    ReceiveAcknowledgement,
    SendData,
    WaitForAcknowledgement
};

struct Session {
    int clientTID = -1;
    int serverTID = -1;
    char clientIP[INET_ADDRSTRLEN] = {0};
    int clientPort = -1;
};

bool str_is_digits_only(std::string str);