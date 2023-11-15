#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
    int block_size = 512;
    int timeout = -1;
    int tsize = -1;
    Direction direction;
    std::ofstream file;
    TransferState current_state;

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

