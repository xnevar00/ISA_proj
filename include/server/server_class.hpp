// File:    server_class.hpp
// Author:  Veronika Nevarilova
// Date:    11/2023

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

void signalHandler(int signal);

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
        std::vector<char> last_data;
        int attempts_to_resend;
        std::string full_filepath;
        int64_t set_timeout_by_client;

        ClientHandler() : current_state(TransferState::WaitForTransfer), clientPort(-1), clientIP(""), direction(-1), block_number(0), tsize(-1), timeout(INITIALTIMEOUT), block_size(512), block_size_set(false), last_packet(false),
                          root_dirpath(""), udpSocket(-1), r_flag(false), overflow(""), attempts_to_resend(0), full_filepath(""), set_timeout_by_client(INITIALTIMEOUT) {}

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
