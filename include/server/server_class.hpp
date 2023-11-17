#include <iostream>
#include <getopt.h>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <string>
#include <atomic>
#include <signal.h>
#include <csignal>
#include <future>
#include <algorithm>
#include "../../include/packet/tftp-packet-class.hpp"

void signalHandler(int signum);

void setupSignalHandler();

class Server
{

protected:
    Server(){}

    static Server* server_;
    std::vector<std::thread> clientThreads;
    int create_udp_socket();
    int setup_udp_socket(int udpSocket, Server *server);
    int initialize_connection(Server *server);
    int respond_to_client(int udpSocket, const char* message, size_t messageLength, struct sockaddr_in* clientAddr, socklen_t clientAddrLen);
    void server_loop(int udpSocket);

public:
    Server(Server &other) = delete;
    
    int port;
    std::string root_dirpath;


    void operator=(const Server &) = delete;

    static Server *getInstance();
    int parse_arguments(int argc, char* argv[], Server *server);
    int listen(Server *server);
};

class ClientHandler {
    public:
        std::ofstream file;
        std::ifstream downloaded_file;
        std::string filename;
        std::string mode;
        TransferState current_state = TransferState::WaitForTransfer;
        int clientPort;
        std::string clientIP;
        sockaddr_in clientAddr;
        int direction;
        unsigned short block_number;
        int64_t tsize;
        int64_t timeout;
        int64_t block_size;
        bool block_size_set;
        bool last_packet;
        std::string root_dirpath;
        int udpSocket;
        bool r_flag;
        std::string overflow;

        ClientHandler() : current_state(TransferState::WaitForTransfer), clientPort(-1), clientIP(""), direction(-1), block_number(0), tsize(-1), timeout(-1), block_size(512), block_size_set(false), last_packet(false),
                          root_dirpath(""), udpSocket(-1), r_flag(false), overflow("") {}

        void handleClient(std::string receivedMessage, int bytesRead, sockaddr_in clientAddr, std::string root_dirpath);
        void handlePacket(TFTPPacket *packet);
        int createUdpSocket();
        int transferFile();
        int receiveData();
        int writeData(std::vector<char> data);
        int setupFileForUpload();
        int setupFileForDownload();
        int handleSendingData();

};
