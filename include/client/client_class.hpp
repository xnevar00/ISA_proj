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

public:
    Client(Client &other) = delete;
    
    int port;
    std::string hostname;
    std::string filepath;
    std::string destFilepath;
    unsigned short int block_number = 0;
    sockaddr_in serverAddr;
    int serverTID = -1;
    int block_size = 512;
    int timeout = -1;
    int tsize = -1;
    ClientSession session;
    bool last_packet = false;
    Direction direction;
    std::ofstream file;
    TransferState current_state = TransferState::WaitForTransfer;

    void operator=(const Client &) = delete;

    static Client *getInstance();
    int parse_arguments(int argc, char* argv[]);
    int create_udp_socket();
    int receive_respond_from_server();
    int send_broadcast_message();
    int communicate();
    void updateAcceptedOptions(TFTPPacket *packet);
    int transferFile();
    int sendAck();
    int receiveData();
    void writeData(std::vector<char> data);
    int setupFileForDownload();
    int sendData();
    int receiveAck();
};

