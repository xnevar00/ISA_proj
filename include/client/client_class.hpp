// File:    client_class.hpp
// Author:  Veronika Nevarilova
// Date:    11/2023

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include <vector>
#include "../../include/packet/tftp-packet-class.hpp"

class Client
{
    protected:
        Client(){}
        static Client* client_;

        int port;
        std::string hostname;
        std::string filepath;
        std::string destFilepath;
        int udpSocket;
        sockaddr_in serverAddr;
        bool last_packet;
        unsigned short int block_number;
        int block_size;
        int timeout;
        int tsize;
        Direction direction;
        std::ofstream file;
        TransferState current_state;
        int serverPort;
        std::string serverIP;
        int clientPort;
        std::string mode;
        bool r_flag;
        std::string overflow;
        int attempts_to_resend;
        std::vector<char> last_data;

        int createUdpSocket();
        int transferData();
        int sendBroadcastMessage();
        void updateAcceptedOptions(TFTPPacket *packet);
        int transferFile();
        int setupFileForDownload();
        int handleSendingData();

    public:
        Client(Client &other) = delete;
        void operator=(const Client &) = delete;
        static Client *getInstance();
        
        int parse_arguments(int argc, char* argv[]);
        int communicate();
};

