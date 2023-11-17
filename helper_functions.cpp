#include "helper_functions.hpp"
#include <cerrno>

bool str_is_digits_only(std::string str) {
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

std::string getArgument(int startIndex, std::string receivedMessage)
{
    if (startIndex >= (int)receivedMessage.length() || startIndex < 0) {
        return "";
    }
    size_t nullByteIndex = receivedMessage.find('\0', startIndex);
    if (nullByteIndex != std::string::npos) {
        std::string result = receivedMessage.substr(startIndex, nullByteIndex - startIndex);
        return result;
    } else {
        return "";
    }
}

int getAnotherStartIndex(int startIndex, std::string receivedMessage)
{
    size_t nullByteIndex = receivedMessage.find('\0', startIndex);
    if (nullByteIndex != std::string::npos) {
        return nullByteIndex + 1;
    } else {
        return -1;
    }
}

std::string getSingleArgument(int startIndex, std::string receivedMessage)
{
    std::string argument = getArgument(startIndex, receivedMessage);
    if (argument == "")
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return "";
    }
    std::cout << "Argument: " << argument << std::endl;
    return argument;
}

int getOpcode(std::string receivedMessage)
{
    int opcode = -1;
    std::string opcode_str = receivedMessage.substr(0, 2);
    opcode = (static_cast<unsigned char>(opcode_str[0]) << 8) | static_cast<unsigned char>(opcode_str[1]);
    return opcode;
}

int setOption(int64_t *option, int *optionIndex, std::string receivedMessage)
{
    *optionIndex = getAnotherStartIndex(*optionIndex, receivedMessage);
    std::string option_str = getSingleArgument(*optionIndex, receivedMessage);
    if (option_str == "" || !str_is_digits_only(option_str))
    {
        return -1;
    } else
    {
        *option = std::stoll(option_str);;
        return 0;
    }
}

std::vector<char> intToBytes(unsigned short value) {

    std::vector<char> bytes(2);
    bytes[0] = static_cast<char>((value >> 8) & 0xFF); // První byte (nejstarší bity)
    bytes[1] = static_cast<char>(value & 0xFF);        // Druhý byte (mladší bity)
    return bytes;
}

std::string getIPAddress(const struct sockaddr_in& sockaddr) {
    char ipBuffer[INET_ADDRSTRLEN];

    // Using inet_ntop for IPv4
    if (inet_ntop(AF_INET, &(sockaddr.sin_addr), ipBuffer, INET_ADDRSTRLEN) != nullptr) {
        return std::string(ipBuffer);
    } else {
        std::cout << "Error converting IP address." << std::endl;
        return "";  // Return an empty string or handle the error accordingly
    }
}

void printAckInfo(std::string src_ip, int src_port, int block_id) {
    std::cerr << "ACK " << src_ip << ":" << src_port << " " << block_id << std::endl;
}

void printDataInfo(std::string src_ip, int src_port, int dst_port, int block_id) {
    std::cerr << "DATA " << src_ip << ":" << src_port << ":" << dst_port << " " << block_id << std::endl;
}

void printErrorInfo(std::string src_ip, int src_port, int dst_port, unsigned short error_code, std::string error_msg) {
    std::cerr << "ERROR " << src_ip << ":" << src_port << ":" << dst_port << " " << error_code << " \"" << error_msg  << "\""<< std::endl;
}

void printOackInfo(std::string src_ip, int src_port, std::string options) {
    std::cerr << "OACK " << src_ip << ":" << src_port << options << std::endl;
}

void printRrqWrqInfo(int opcode, std::string src_ip, int src_port, std::string filepath, std::string mode, std::string options) {
    std::string output_opcode = "";
    if (opcode == Opcode::WRQ)
    {
        output_opcode = "WRQ ";
    } else if (opcode == Opcode::RRQ)
    {
        output_opcode = "RRQ ";
    } else
    {
        return;
    }
    std::cerr << output_opcode << src_ip << ":" << src_port << " \"" << filepath << "\" " << mode << " " << options << std::endl;
}

int getLocalPort(int udpSocket) {
    struct sockaddr_in localAddress;
    socklen_t addressLength = sizeof(localAddress);
    getsockname(udpSocket, (struct sockaddr*)&localAddress, &addressLength);
    return ntohs(localAddress.sin_port);
}

int getPort(const struct sockaddr_in& sockaddr)
{
    return ntohs(sockaddr.sin_port);
}
