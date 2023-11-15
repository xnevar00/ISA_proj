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
#include "../../include/packet/tftp-packet-class.hpp"

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
        Session session;
        std::ofstream file;
        std::ifstream downloaded_file;
        TransferState current_state = TransferState::WaitForTransfer;
        sockaddr_in clientAddr;
        int clientPort;
        std::string clientIP;

        ClientHandler(){};
        void handleClient(std::string receivedMessage, int bytesRead, sockaddr_in clientAddr, socklen_t clientAddrLen, std::string root_dirpath);
        void handlePacket(TFTPPacket *packet);
        int createUdpSocket();
        int transferFile();
        int receiveData();
        void writeData(std::vector<char> data);
        int setupFileForUpload();
        int setupFileForDownload();
        int handleSendingData();

};
