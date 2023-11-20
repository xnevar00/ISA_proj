/**
 * @file helper_functions.hpp
 * @author Veronika Nevarilova (xnevar00)
 * @date 11/2023
 */

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <fstream>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>
#include <filesystem>
#include <cerrno>
#include <cstdio>
#include <sys/statvfs.h>
#include <cstdint>
#include "output_handler/output_handler.hpp"

#define IPADDRLEN 16
#define MINPORTVALUE 1
#define MAXPORTVALUE 65535
#define MAXTSIZEVALUE 9223372036854775807
#define MAXBLKSIZEVALUE 65464
#define MAXTIMEOUTVALUE 255
#define MAXRESENDATTEMPTS 4
#define INITIALTIMEOUT 2
#define MAXOPTIONNUMERALS 19
#define MAXMESSAGESIZE 65507

// opcodes of the packets
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
    PARSING_ERROR = -1,
    UNK_TID = -2,
    TIMEOUT = -3,
    RECV_ERR = -4,
    OTHER = -5,
};

// states for the finite state machine
enum TransferState {
    WaitForTransfer,
    SendAck,
    ReceiveData,
    SendData,
    ReceiveAck,
};

//upload = client sends WRQ, download = client sends RRQ
enum Direction {
    Upload,
    Download
}; 


/**
 * @brief Checks if a string contains only digits.
 *
 * @param str String to be checked.
 * @return True if the string contains only digits, false otherwise
 */
bool str_is_digits_only(std::string str);


/**
 * @brief Function that is called by the getSingleArgument function, help to get the argument
 *
 * @param startIndex Starting index of the argument to be extracted.
 * @param receivedMessage Original message.
 * @return The extracted argument as a string.
 */
std::string getArgument(int startIndex, std::string receivedMessage);


/**
 * @brief Gets the index of the next argument in the message based on the null byte
 *
 * @param startIndex Starting index of the argument to be extracted.
 * @param receivedMessage Original message.
 * @return Index of the next argument.
 */
int getAnotherStartIndex(int startIndex, std::string receivedMessage);


/**
 * @brief Extracts a single argument from a received message starting from a specific index
 *        and makes some additional checks.
 *
 * @param startIndex Starting index of the argument to be extracted.
 * @param receivedMessage Original message.
 * @return The extracted argument as a string.
 */
std::string getSingleArgument(int startIndex, std::string receivedMessage); 


/**
 * @brief Gets the opcode from a received message (first two bytes).
 *
 * @param receivedMessage Original message.
 * @return Opcode as an integer.
 */
int getOpcode(std::string receivedMessage);


/**
 * @brief Sets the option to the value from the received message.
 *
 * @param option Pointer to the option variable, where the value will be stored.
 * @param optionIndex Pointer to the index of the option in the message.
 * @param receivedMessage Original message.
 * @param options_string String containing all the options for printing to stderr after parsing the packet.
 * @return 0 if the option was set successfully, -1 otherwise.
 */
int setOption(int64_t *option, int *optionIndex, std::string receivedMessage, std::string *options_string);


/**
 * @brief Converts an integer to a vector of bytes.
 *
 * @param value Integer to be converted.
 * @return Vector of bytes.
 */
std::vector<char> intToBytes(unsigned short value);


/**
 * @brief Gets the IP address from a sockaddr_in structure.
 *
 * @param sockaddr sockaddr_in structure.
 * @return IP address as a string.
 */
std::string getIPAddress(const struct sockaddr_in& sockaddr);


/**
 * @brief Prints the information about a received ACK packet to stderr.
 *
 * @param src_ip IP address of the sender.
 * @param src_port Port of the sender.
 * @param block_id Block number of the ACK packet.
 */
void printAckInfo(std::string src_ip, int src_port, int block_id);


/**
 * @brief Prints the information about a received DATA packet to stderr.
 *
 * @param src_ip IP address of the sender.
 * @param src_port Port of the sender.
 * @param dst_port Port of the receiver.
 * @param block_id Block number of the DATA packet.
 */
void printDataInfo(std::string src_ip, int src_port, int dst_port, int block_id);


/**
 * @brief Prints the information about a received ERROR packet to stderr.
 *
 * @param src_ip IP address of the sender.
 * @param src_port Port of the sender.
 * @param dst_port Port of the receiver.
 * @param error_code Error code of the ERROR packet.
 * @param error_msg Error message of the ERROR packet.
 */
void printErrorInfo(std::string src_ip, int src_port, int dst_port, unsigned short error_code, std::string error_msg);


/**
 * @brief Prints the information about a received OACK packet to stderr.
 *
 * @param src_ip IP address of the sender.
 * @param src_port Port of the sender.
 * @param options String containing all the options.
 */
void printOackInfo(std::string src_ip, int src_port, std::string options);


/**
 * @brief Prints the information about a received RRQ/WRQ packet to stderr.
 *
 * @param opcode Opcode of the packet.
 * @param src_ip IP address of the sender.
 * @param src_port Port of the sender.
 * @param filepath Requested file.
 * @param mode Requested mode.
 * @param options String containing all the options.
 */
void printRrqWrqInfo(int opcode, std::string src_ip, int src_port, std::string filepath, std::string mode, std::string options);


/**
 * @brief Gets the local port of a socket.
 *
 * @param udpSocket Socket.
 * @return Port of the socket.
 */
int getLocalPort(int udpSocket);


/**
 * @brief Gets the port from a sockaddr_in structure.
 *
 * @param sockaddr sockaddr_in structure.
 * @return Port.
 */
int getPort(const struct sockaddr_in& sockaddr);


/**
 * @brief Sets the timeout of a socket.
 *
 * @param udp_socket Pointer to the socket.
 * @param seconds Timeout in seconds.
 * @return 0 if the timeout was set successfully, -1 otherwise.
 */
int setTimeout(int *udp_socket, int seconds);


/**
 * @brief Resends the data to the destination.
 *
 * @param udp_socket Socket.
 * @param destination Destination address.
 * @param data Data to be resent.
 * @return 0 if the data were resent successfully, -1 otherwise.
 */
int resendData(int udp_socket, sockaddr_in destination, std::vector<char> data);


/**
 * @brief Closes the file and deletes it.
 *
 * @param file Pointer to the file.
 * @param file_path Path to the file.
 */
void clean(std::ofstream *file, std::string full_filepath);


/**
 * @brief Gets the size of a file.
 *
 * @param root_ditpath Path to the root directory.
 * @param filepath Path to the file.
 * @return Size of the file.
 */
int getFileSize(std::string root_ditpath, std::string filepath);


/**
 * @brief Checks if there is enough space on the disk for the requested file
 *
 * @param root_dirpath Path to the root directory.
 * @param tsize Size of the file.
 * @return 1 if there is enough space, 0 otherwise and -1 if error occured.
 */
int isEnoughSpace(std::string root_dirpath, int64_t tsize);