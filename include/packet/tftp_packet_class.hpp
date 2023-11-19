/**
 * @file tftp_packet_class.hpp
 * @author Veronika Nevarilova (xnevar00)
 * @date 11/2023
 */

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
#include <cstring>
#include <algorithm>

/**
 * @class TFTPPacket
 * @brief A class that represents a TFTP packet.
 *
 * This class provides methods to parse a received message into a TFTP packet and to send a TFTP packet.
 */
class TFTPPacket {
public:
    int opcode;                     // opcode of the packet
    std::string filename;           // filename of the file to be transferred (used in RRQ and WRQ packet)
    std::string mode;               // mode of the transfer (used in RRQ and WRQ packet)
    int64_t timeout = -1;           // timeout of the transfer (used in RRQ and WRQ packet)
    int64_t blksize = -1;           // blocksize of the transfer (used in RRQ and WRQ packet)
    int64_t tsize = -1;             // tsize of the transfer (used in RRQ and WRQ packet)
    bool blksize_set = false;       // flag that indicates whether the blocksize was set (used in RRQ and WRQ packet)
    unsigned short blknum;          // block number of the packet (used in ACK and DATA packet)
    std::vector<char> data;         // data of the packet (used in DATA packet)
    unsigned short error_code;      // error code of the packet (used in ERROR packet)
    std::string error_message;      // error message of the packet (used in ERROR packet)
    std::string options_string;     // string containing all the options (used in RRQ, WRQ and OACK packet)

    virtual ~TFTPPacket() {}        // virtual destructor

    /**
     * @brief Parses a received message into a TFTP packet.
     *
     * @param receivedMessage Received message.
     * @return int Returns 0 if the parsing was successful.
     */
    virtual int parse(std::string receivedMessage) = 0;

    /**
     * @brief Sends a TFTP packet.
     * @param udpSocket The UDP socket to send the packet on.
     * @param destination The destination to send the packet to.
     * @param last_data The last data sent.
     * @return 0 if successful.
     */
    virtual int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const = 0;

    /**
     * @brief Parses a received message into a TFTP packet.
     *
     * @param receivedMessage Received message.
     * @param srcIP IP address of the sender.
     * @param srcPort Port of the sender.
     * @param dstPort Port of the receiver.
     * @return Returns a pair of a TFTP packet and an error code.
     */
    static std::pair<TFTPPacket *, int> parsePacket(std::string receivedMessage, std::string srcIP, int srcPort, int dstPort);
    
    /**
     * @brief Sends an ACK packet.
     *
     * @param block_number Block number of the ACK packet.
     * @param udp_socket UDP socket to send the ACK packet on.
     * @param addr Address of the receiver.
     * @param last_data Sent message will be stored in this variable (used for resending the message in case of timeout)
     * @return 0 if successful.
     */
    static int sendAck(int block_number, int udp_socket, sockaddr_in addr, std::vector<char> *last_data);

    /**
     * @brief Receives an ACK packet.
     *
     * @param udp_socket UDP socket to receive the ACK packet on.
     * @param block_number Block number of the ACK packet.
     * @param client_port Port of the receiver.
     * @return 0 if successful.
     */
    static int receiveAck(int udp_socket, short unsigned block_number, int client_port);

    /**
     * @brief Receives a DATA packet.
     *
     * @param udp_socket UDP socket to receive the DATA packet on.
     * @param block_number Block number of the DATA packet.
     * @param block_size Block size of the DATA packet.
     * @param file File to write the data to.
     * @param last_packet Flag that indicates whether the last packet was received.
     * @param client_port Port of the receiver.
     * @param r_flag Flag that indicates whether in the last packet received at the last index was a '\r' character (used in netascii mode).
     * @param mode Mode of the transfer.
     * @return 0 if successful.
     */
    static int receiveData(int udp_socket, int block_number, int block_size, std::ofstream *file, bool *last_packet, int client_port, bool *r_flag, std::string mode);
    
    /**
     * @brief Sends a DATA packet.
     *
     * @param udp_socket UDP socket to send the DATA packet on.
     * @param addr Address of the receiver.
     * @param block_number Block number of the DATA packet.
     * @param block_size Block size of the DATA packet.
     * @param bytes_read Number of bytes read from the file.
     * @param data Data to be sent.
     * @param last_packet Flag that indicates whether this is the last packet.
     * @param last_data Sent message will be stored in this variable (used for resending the message in case of timeout)
     * @return 0 if successful.
     */
    static int sendData(int udp_socket, sockaddr_in addr, int block_number, int block_size, int bytes_read, std::vector<char> data, bool *last_packet, std::vector<char> *last_data);
    
    /**
     * @brief Sends an ERROR packet.
     *
     * @param udp_socket UDP socket to send the ERROR packet on.
     * @param addr Address of the receiver.
     * @param error_code Error code of the ERROR packet.
     * @param error_message Error message of the ERROR packet.
     * @return 0 if successful.
     */
    static int sendError(int udp_socket, sockaddr_in addr, int error_code, std::string error_message);
};


/**
 * @class RRQWRQPacket
 * @brief A class that represents a Read Request or Write Request TFTP packet.
 *
 * This class provides methods to parse a received message into a Read Request or Write Request packet and to send it.
 */
class RRQWRQPacket : public TFTPPacket {
public:
    RRQWRQPacket() {};
    RRQWRQPacket(int opcode, std::string filename, std::string mode, int timeout, int blksize, int tsize);

    // methods description in TFTPPacket class
    int parse(std::string receivedMessage) override;
    int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const override;
    
};

/**
 * @class DATAPacket
 * @brief A class that represents a Data TFTP packet.
 *
 * This class provides methods to parse a received message into a Data packet and to send it.
 */
class DATAPacket : public TFTPPacket {
public:
    DATAPacket() {};
    DATAPacket(unsigned short blknum, std::vector<char> data);

    // methods description in TFTPPacket class
    int parse(std::string receivedMessage) override;
    int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const override;
};

/**
 * @class ACKPacket
 * @brief A class that represents an Acknowledgement TFTP packet.
 *
 * This class provides methods to parse a received message into an Acknowledgement packet and to send it.
 */
class ACKPacket : public TFTPPacket {
public:
    ACKPacket() {};
    ACKPacket(unsigned short blknum);

    // methods description in TFTPPacket class
    int parse(std::string receivedMessage) override;
    int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const override;
};

/**
 * @class OACKPacket
 * @brief A class that represents an Option Acknowledgement TFTP packet.
 *
 * This class provides methods to parse a received message into an Option Acknowledgement packet and to send it.
 */
class OACKPacket : public TFTPPacket {
public:
    OACKPacket() {};
    OACKPacket(unsigned short blknum, bool blocksize_set, int blocksize, int timeout, int tsize);

    // methods description in TFTPPacket class
    int parse(std::string receivedMessage) override;
    int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const override;
};

/**
 * @class ERRORPacket
 * @brief A class that represents an Error TFTP packet.
 *
 * This class provides methods to parse a received message into an Error packet and to send it.
 */
class ERRORPacket : public TFTPPacket {
public:
    ERRORPacket() {};
    ERRORPacket(unsigned short error_code, std::string error_msg);

    // methods description in TFTPPacket class
    int parse(std::string receivedMessage) override;
    int send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const override;
};