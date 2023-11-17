#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <fstream>
#include <vector>
#include <arpa/inet.h>
#include <filesystem>

#define IPADDRLEN 16
#define MINPORTVALUE 1
#define MAXPORTVALUE 65535

enum Opcode {
    RRQ = 1,
    WRQ = 2,
    DATA = 3,
    ACK = 4,
    ERROR = 5,
    OACK = 6
};

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

bool str_is_digits_only(std::string str);
std::string getArgument(int startIndex, std::string receivedMessage);
int getAnotherStartIndex(int startIndex, std::string receivedMessage);
std::string getSingleArgument(int startIndex, std::string receivedMessage); 
int getOpcode(std::string receivedMessage);
int setOption(int *option, int *optionIndex, std::string receivedMessage);
std::vector<char> intToBytes(unsigned short value);
std::string getIPAddress(const struct sockaddr_in& sockaddr);
void printAckInfo(std::string src_ip, int src_port, int block_id);
void printDataInfo(std::string src_ip, int src_port, int dst_port, int block_id);
void printErrorInfo(std::string src_ip, int src_port, int dst_port, unsigned short error_code, std::string error_msg);
void printOackInfo(std::string src_ip, int src_port, std::string options);
void printRrqWrqInfo(int opcode, std::string src_ip, int src_port, std::string filepath, std::string mode, std::string options);
int getLocalPort(int udpSocket);
int getPort(const struct sockaddr_in& sockaddr);