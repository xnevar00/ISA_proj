#include <memory>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../../helper_functions.hpp"
#include <cstdlib>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include <vector>

enum Opcode {
    RRQ = 1,
    WRQ = 2,
    DATA = 3,
    ACK = 4,
    ERROR = 5,
    OACK = 6
};

class TFTPPacket {
public:
    int opcode;
    std::string filename;
    std::string mode;
    int timeout = -1;
    int blksize = -1;
    int tsize = -1;
    bool blksize_set = false;
    unsigned short blknum;
    std::vector<char> data;
    std::string errorcode;
    std::string errormsg;

    virtual int parse(std::string receivedMessage) = 0;
    virtual std::string create(Session* session) const = 0;
    virtual int send(int udpSocket, sockaddr_in destination) const = 0;
    static TFTPPacket *parsePacket(std::string receivedMessage);
};

class RRQWRQPacket : public TFTPPacket {
public:
    RRQWRQPacket() {};
    RRQWRQPacket(int opcode, std::string filename, std::string mode, int timeout, int blksize, int tsize);
    int parse(std::string receivedMessage) override;
    std::string create(Session* session) const override;
    int send(int udpSocket, sockaddr_in destination) const override;
    
};

class DATAPacket : public TFTPPacket {
public:
    DATAPacket() {};
    DATAPacket(unsigned short blknum, std::vector<char> data);
    int parse(std::string receivedMessage) override;
    std::string create(Session* session) const override;
    int send(int udpSocket, sockaddr_in destination) const override;
};

class ACKPacket : public TFTPPacket {
public:
    ACKPacket() {};
    ACKPacket(unsigned short blknum);

    int parse(std::string receivedMessage) override;
    std::string create(Session* session) const override;
    int send(int udpSocket, sockaddr_in destination) const override;
};

class OACKPacket : public TFTPPacket {
public:
    OACKPacket() {};
    OACKPacket(unsigned short blknum, bool blocksize_set, int blocksize, int timeout, int tsize);
    int parse(std::string receivedMessage) override;
    std::string create(Session* session) const override;
    int send(int udpSocket, sockaddr_in destination) const override;
};

class ERRORPacket : public TFTPPacket {
public:
    ERRORPacket() {};
    int parse(std::string receivedMessage) override;
    std::string create(Session* session) const override;
    int send(int udpSocket, sockaddr_in destination) const override;

};