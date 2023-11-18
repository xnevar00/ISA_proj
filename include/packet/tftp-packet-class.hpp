// File:    tftp-packet-class.hpp
// Author:  Veronika Nevarilova
// Date:    11/2023

#include <memory>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../helper_functions/helper_functions.hpp"
#include <cstdlib>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include <vector>

class TFTPPacket {
public:
    int opcode;
    std::string filename;
    std::string mode;
    int64_t timeout = -1;
    int64_t blksize = -1;
    int64_t tsize = -1;
    bool blksize_set = false;
    unsigned short blknum;
    std::vector<char> data;
    unsigned short error_code;
    std::string error_message;
    std::string options_string;

    virtual int parse(std::string receivedMessage) = 0;
    virtual int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const = 0;
    static std::pair<TFTPPacket *, int> parsePacket(std::string receivedMessage, std::string srcIP, int srcPort, int dstPort);
    static int sendAck(int block_number, int udp_socket, sockaddr_in addr, std::vector<char> *last_data);
    static int receiveAck(int udp_socket, short unsigned block_number, int client_port);
    static int receiveData(int udp_socket, int block_number, int block_size, std::ofstream *file, bool *last_packet, int client_port, bool *r_flag, std::string mode);
    static int sendData(int udp_socket, sockaddr_in addr, int block_number, int block_size, int bytes_read, std::vector<char> data, bool *last_packet, std::vector<char> *last_data);
    static int sendError(int udp_socket, sockaddr_in addr, int error_code, std::string error_message);
};

class RRQWRQPacket : public TFTPPacket {
public:
    RRQWRQPacket() {};
    RRQWRQPacket(int opcode, std::string filename, std::string mode, int timeout, int blksize, int tsize);
    int parse(std::string receivedMessage) override;
    int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const override;
    
};

class DATAPacket : public TFTPPacket {
public:
    DATAPacket() {};
    DATAPacket(unsigned short blknum, std::vector<char> data);
    int parse(std::string receivedMessage) override;
    int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const override;
};

class ACKPacket : public TFTPPacket {
public:
    ACKPacket() {};
    ACKPacket(unsigned short blknum);

    int parse(std::string receivedMessage) override;
    int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const override;
};

class OACKPacket : public TFTPPacket {
public:
    OACKPacket() {};
    OACKPacket(unsigned short blknum, bool blocksize_set, int blocksize, int timeout, int tsize);
    int parse(std::string receivedMessage) override;
    int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const override;
};

class ERRORPacket : public TFTPPacket {
public:
    ERRORPacket() {};
    ERRORPacket(unsigned short error_code, std::string error_msg);
    int parse(std::string receivedMessage) override;
    int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const override;

};