#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

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

enum Direction {
    Upload,
    Download
}; 

enum Mode {
    NETASCII,
    OCTET
};

struct Session {
    int direction = -1;
    int clientTID = -1;
    int serverTID = -1;
    char clientIP[INET_ADDRSTRLEN] = {0};
    int clientPort = -1;
    std::string filename;
    std::string mode;
    bool blksize_option = false;
    int blksize = -1;
    bool timeout_option = false;
    int timeout = -1;  
    bool tsize_option = false;
    int tsize = -1;
    int udpSocket = -1;
    struct sockaddr_in serverAddr;
};

bool str_is_digits_only(std::string str);

std::string getArgument(int startIndex, std::string receivedMessage);
int getAnotherStartIndex(int startIndex, std::string receivedMessage);
std::string getSingleArgument(int startIndex, std::string receivedMessage);
int getPairArgument(int optionIndex, std::string receivedMessage, Session *session);
void setMode(std::string mode, Session *session);