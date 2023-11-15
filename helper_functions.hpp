#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <fstream>
#include <vector>

#define IPADDRLEN 16
#define MINPORTVALUE 1
#define MAXPORTVALUE 65535

enum StatusCode {
    SUCCESS = 0,
    INVALID_ARGUMENTS = -1,
    SOCKET_ERROR = -2,
    CONNECTION_ERROR = -3,
    PACKET_ERROR = -4,
};

enum class TransferState {
    WaitForTransfer,
    SendAck,
    ReceiveData,
    SendData,
    ReceiveAck,
    SendError
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
    std::string filename = "";
    std::string mode;
    int blksize = 512;
    bool blksize_set = false;
    int timeout = -1;
    int tsize = -1;
    int udpSocket = -1;
    struct sockaddr_in clientAddr;
    unsigned short block_number = 0;
    bool last_packet = false;
    std::string latest_data = "";
    std::string root_dirpath = "";
    std::ofstream file;
};

struct ClientSession {
    //int serverTID = -1;
    int udpSocket = -1;
    struct sockaddr_in serverAddr;
    int initial_port = -1;
    std::string hostname = "";
    std::string filepath = "";
    std::string destFilepath = "";
};

bool str_is_digits_only(std::string str);

std::string getArgument(int startIndex, std::string receivedMessage);
int getAnotherStartIndex(int startIndex, std::string receivedMessage);
std::string getSingleArgument(int startIndex, std::string receivedMessage);
int getPairArgument(int optionIndex, std::string receivedMessage, Session *session);
int getOpcode(std::string receivedMessage);
int setOption(int *option, int *optionIndex, std::string receivedMessage);
std::vector<char> intToBytes(unsigned short value);