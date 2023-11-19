/**
 * @file helper_functions.cpp
 * @author Veronika Nevarilova (xnevar00)
 * @date 11/2023
 */


#include "../../include/helper_functions/helper_functions.hpp"


bool str_is_digits_only(std::string str) {
    for (char c : str) 
    {
        if (!std::isdigit(c)) 
        {
            return false;
        }
    }
    return true;
}

std::string getArgument(int startIndex, std::string receivedMessage)
{
    if (startIndex >= (int)receivedMessage.length() || startIndex < 0) 
    {
        return "";
    }
    // find the first null byte after the start index
    size_t nullByteIndex = receivedMessage.find('\0', startIndex);
    if (nullByteIndex != std::string::npos) 
    {
        std::string result = receivedMessage.substr(startIndex, nullByteIndex - startIndex);
        return result;
    } else 
    {
        return "";
    }
}

int getAnotherStartIndex(int startIndex, std::string receivedMessage)
{
    size_t nullByteIndex = receivedMessage.find('\0', startIndex);
    if (nullByteIndex != std::string::npos && nullByteIndex + 1 < receivedMessage.length()) 
    {
        //start index if always after zero byte
        return nullByteIndex + 1;
    } else 
    {
        return -1;
    }
}

std::string getSingleArgument(int startIndex, std::string receivedMessage)
{
    std::string argument = getArgument(startIndex, receivedMessage);
    if (argument == "")
    {
        OutputHandler::getInstance()->print_to_cout("Error getting argument.");
        return "";
    }
    OutputHandler::getInstance()->print_to_cout("Argument: " + argument);
    return argument;
}

int getOpcode(std::string receivedMessage)
{
    int opcode = -1;
    // opcode is in the first two bytes of 
    std::string opcode_str = receivedMessage.substr(0, 2);
    // convert to int
    opcode = (static_cast<unsigned char>(opcode_str[0]) << 8) | static_cast<unsigned char>(opcode_str[1]);
    return opcode;
}

int setOption(int64_t *option, int *optionIndex, std::string receivedMessage, std::string *options_string)
{
    *optionIndex = getAnotherStartIndex(*optionIndex, receivedMessage);
    std::string option_str = getSingleArgument(*optionIndex, receivedMessage);
    if (option_str == "" || !str_is_digits_only(option_str)) 
    {
        return -1;
    } else 
    {
        if (option_str.length() > MAXOPTIONNUMERALS)
        {
            *option = MAXTSIZEVALUE + 1; // set to invalid value
        } else 
        {
            *option = std::stoll(option_str);
        }
        *options_string += "=" + option_str;
        return 0;
    }
}

std::vector<char> intToBytes(unsigned short value) 
{

    std::vector<char> bytes(2);
    bytes[0] = static_cast<char>((value >> 8) & 0xFF); // first byte
    bytes[1] = static_cast<char>(value & 0xFF);        // second byte
    return bytes;
}

std::string getIPAddress(const struct sockaddr_in& sockaddr) 
{
    char ipBuffer[INET_ADDRSTRLEN];

    if (inet_ntop(AF_INET, &(sockaddr.sin_addr), ipBuffer, INET_ADDRSTRLEN) != nullptr) 
    {
        return std::string(ipBuffer);
    } else {
        OutputHandler::getInstance()->print_to_cerr("Error converting IP address.");
        return "";
    }
}

void printAckInfo(std::string src_ip, int src_port, int block_id) 
{
    OutputHandler::getInstance()->print_to_cerr("ACK " + src_ip + ":" + std::to_string(src_port) + " " + std::to_string(block_id));
}

void printDataInfo(std::string src_ip, int src_port, int dst_port, int block_id) 
{
    OutputHandler::getInstance()->print_to_cerr("DATA " + src_ip + ":" + std::to_string(src_port) + ":" + std::to_string(dst_port) + " " + std::to_string(block_id));
}

void printErrorInfo(std::string src_ip, int src_port, int dst_port, unsigned short error_code, std::string error_msg) 
{
    OutputHandler::getInstance()->print_to_cerr("ERROR " + src_ip + ":" + std::to_string(src_port) + ":" + std::to_string(dst_port) + " " + std::to_string(error_code) + " \"" + error_msg  + "\"");
}

void printOackInfo(std::string src_ip, int src_port, std::string options) 
{
    OutputHandler::getInstance()->print_to_cerr("OACK " + src_ip + ":" + std::to_string(src_port) + " " + options);
}

void printRrqWrqInfo(int opcode, std::string src_ip, int src_port, std::string filepath, std::string mode, std::string options) 
{
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
    OutputHandler::getInstance()->print_to_cerr(output_opcode + src_ip + ":" + std::to_string(src_port) + " \"" + filepath + "\" " + mode + options);
}

int getLocalPort(int udpSocket) 
{
    struct sockaddr_in localAddress;
    socklen_t addressLength = sizeof(localAddress);
    getsockname(udpSocket, (struct sockaddr*)&localAddress, &addressLength);
    return ntohs(localAddress.sin_port);
}

int getPort(const struct sockaddr_in& sockaddr)
{
    return ntohs(sockaddr.sin_port);
}

int setTimeout(int *udp_socket, int seconds)
{
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    return setsockopt(*udp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

int resendData(int udp_socket, sockaddr_in destination, std::vector<char> data)
{
    if (sendto(udp_socket, data.data(), data.size(), 0, (struct sockaddr*)&destination, sizeof(destination)) == -1) {
        OutputHandler::getInstance()->print_to_cerr("Error while resending the message:" + std::to_string(errno));
        return -1;
    }
    return 0;
}

void clean(std::ofstream *file, std::string file_path)
{
    if (file->is_open())
    {
        file->close();
    }

    // tries to delete the file
    if (remove(file_path.c_str()) != 0) {
        OutputHandler::getInstance()->print_to_cout("Error deleting file.");
    }
}

int getFileSize(std::string root_ditpath, std::string filepath)
{
    std::string full_filepath = root_ditpath + "/" + filepath;
    std::ifstream file(full_filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return -1;
    }
    int size = file.tellg();
    file.close();
    return size;
}

int isEnoughSpace(std::string root_dirpath, int64_t tsize)
{
    struct statvfs stat;

    if (statvfs(root_dirpath.c_str(), &stat) != 0) {
        // error happens, just quits here
        OutputHandler::getInstance()->print_to_cout("Failed to get disk space: " + std::to_string(errno));
        return -1;
    }

    // the available size is f_bsize * f_bavail
    uint64_t available = stat.f_bsize * stat.f_bavail;

    if (available >= static_cast<uint64_t>(tsize)) {
        return 1;  // enough space
    } else {
        return 0;  // not enough space
    }
}