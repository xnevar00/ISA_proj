#include "../../include/packet/tftp-packet-class.hpp"
#include <cstring>


TFTPPacket* TFTPPacket::parsePacket(std::string receivedMessage, std::string src_IP, int src_port, int dst_port)
{
    int opcode = getOpcode(receivedMessage);
    TFTPPacket* packet;
    switch(opcode){
        case 1:
            packet = new RRQWRQPacket();
            break;
        case 2:
            packet = new RRQWRQPacket();
            break;
        case 3:
            packet = new DATAPacket();
            break;
        case 4:
            packet = new ACKPacket();
            break;
        case 5:
            packet = new ERRORPacket();
            break;
        case 6:
            packet = new OACKPacket();
            break;
        default :
            return nullptr;
            break;
    }
    if(packet->parse(receivedMessage) == -1)
    {
        std::cout << "chyba parsovani packetu" << std::endl;
        return nullptr;
    }
        switch(opcode){
        case 1:
        case 2:
            printRrqWrqInfo(packet->opcode, src_IP, src_port, packet->filename, packet->mode, "");
            break;
        case 3:
            printDataInfo(src_IP, src_port, dst_port, packet->blknum);
            break;
        case 4:
            printAckInfo(src_IP, src_port, packet->blknum);
            break;
        case 5:
            printErrorInfo(src_IP, src_port, dst_port, packet->error_code, packet->error_message);
            break;
        case 6:
            printOackInfo(src_IP, src_port, "");
            break;
        default :
            break;
    }
    return packet;
}

int TFTPPacket::sendAck(int block_number, int udp_socket, sockaddr_in addr)
{
    ACKPacket response_packet(block_number);
    if (response_packet.send(udp_socket, addr) == -1)
    {
        return -1;
    }
    return 0;
}

int TFTPPacket::receiveAck(int udp_socket, short unsigned block_number)
{
    std::cout << "Waiting for ACK packet..." << std::endl;
    char received_data[65507];
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);

    int bytesRead = recvfrom(udp_socket, received_data, sizeof(received_data), 0, (struct sockaddr*)&addr, &addrLen);
    if (bytesRead == -1) {
        std::cout << "Chyba při čekání na zprávu: " << strerror(errno) << std::endl;
        return -1;
    }

    std::string received_message(received_data, bytesRead);
    std::cout << "Received ACK packet." << std::endl;
    if (bytesRead < 4)
    {
        //TODO error packet
        return -1;
    }

    TFTPPacket *packet = TFTPPacket::parsePacket(received_message, getIPAddress(addr), ntohs(addr.sin_port), getLocalPort(udp_socket));
    if (packet == nullptr)
    {
        return -1;
    }
    if (packet->opcode != Opcode::ACK)
    {
        return -1;
    }
    if (packet->blknum != block_number)
    {
        std::cout << "Wrong block number!" << std::endl;
        return -1;
    }

    return 0;
}

int TFTPPacket::receiveData(int udp_socket, int block_number, int block_size, std::ofstream *file, bool *last_packet)
{
    char received_data[65507]; // Buffer pro přijatou zprávu, max velikost UDP packetu
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    std::cout << "Waiting for DATA packet..." << std::endl;
    int bytesRead = recvfrom(udp_socket, received_data, sizeof(received_data), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (bytesRead == -1) {
        std::cout << "Chyba při čekání na zprávu: " << strerror(errno) << std::endl;
        return -1;
    }

    std::string received_message(received_data, bytesRead);
    if (bytesRead < 4)
    {
        //TODO error packet
        return -1;
    }

    TFTPPacket *packet = TFTPPacket::parsePacket(received_message, getIPAddress(clientAddr), ntohs(clientAddr.sin_port), getLocalPort(udp_socket));
    if (packet == nullptr)
    {
        //TODO error packet
        return -1;
    }
    if (packet->opcode != Opcode::DATA)
    {
        //TODO error packet
        return -1;
    }
    if (packet->blknum != block_number)
    {
        //TODO osetrit
        std::cout << "Wrong block number!" << std::endl;
        return -1;
    }
    file->write(packet->data.data(), packet->data.size());
    if ((unsigned short int)packet->data.size() < block_size)
    {
        *last_packet = true;
        file->close();
    }

    return 0;
}

int TFTPPacket::sendData(int udp_socket, sockaddr_in addr, int block_number, int block_size, int bytes_read, std::vector<char> data, bool *last_packet)
{
    if (bytes_read < 0) {
        std::cout << "Error reading from stdin." << std::endl;
        return -1;
    }

    data.resize(bytes_read);
    if (data.size() < block_size)
    {
        *last_packet = true;
    }
    DATAPacket response_packet(block_number, data);
    if (response_packet.send(udp_socket, addr) == -1)
    {
        std::cout << "Error sending DATA packet." << std::endl;
        return -1;
    }
    return 0;
}

int TFTPPacket::sendError(int udp_socket, sockaddr_in addr, int error_code, std::string error_message)
{
    ERRORPacket error_packet(error_code, error_message);
    if (error_packet.send(udp_socket, addr) == -1)
    {
        return -1;
    }
    return 0;
}

RRQWRQPacket::RRQWRQPacket(int opcode, std::string filename, std::string mode, int timeout, int blksize, int tsize)
{
    this->opcode = opcode;
    this->filename = filename;
    this->mode = mode;
    this->timeout = timeout;
    this->blksize = blksize;
    this->tsize = tsize;
}

int RRQWRQPacket::parse(std::string receivedMessage) {
    if (receivedMessage.length() < 4)
    {
        return -1;
    }
    opcode = getOpcode(receivedMessage);

    int filenameIndex = 2;
    filename = getSingleArgument(filenameIndex, receivedMessage);
    if (filename == "")
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return -1;
    }
    int modeIndex = getAnotherStartIndex(filenameIndex, receivedMessage);
    mode = getSingleArgument(modeIndex, receivedMessage);
    std::cout << "Mode: " << mode << std::endl;
    if (mode != "netascii" && mode != "octet")
    {
        //TODO error packet
        std::cout << "CHYBA2" << std::endl;
        return -1;
    }

    int optionIndex = getAnotherStartIndex(modeIndex, receivedMessage);
    std::string option;
    while(optionIndex != -1 && optionIndex != (int)receivedMessage.length())
    {
        option = getSingleArgument(optionIndex, receivedMessage);
        if (optionIndex == -1)
        {
            //TODO error packet
            std::cout << "CHYBA" << std::endl;
            return -1;
        } else if (option == "blksize")
        {
            if (blksize == -1)
            {
                setOption(&blksize, &optionIndex, receivedMessage);
            } else {
                //TODO error packet
                std::cout << "CHYBA" << std::endl;
                return -1;
            }
        } else if (option == "timeout")
        {
            if (timeout == -1)
            {
                setOption(&timeout, &optionIndex, receivedMessage);
            } else {
                //TODO error packet
                std::cout << "CHYBA" << std::endl;
                return -1;
            }
        } else if (option == "tsize")
        {
            if (tsize == -1)
            {
                setOption(&tsize, &optionIndex, receivedMessage);
            } else {
                //TODO error packet
                std::cout << "CHYBA" << std::endl;
                return -1;
            }
        } else {
            //TODO error packet
            std::cout << "CHYBA" << std::endl;
            return -1;
        }
        optionIndex = getAnotherStartIndex(optionIndex, receivedMessage);

    }
    std::cout << "RRW packet parsed." << std::endl;
    return 0;
}

int RRQWRQPacket::send(int udpSocket, sockaddr_in destination) const {
    std::vector<char> message;
    std::vector<char> opcode = intToBytes(this->opcode);
    message.insert(message.end(), opcode.begin(), opcode.end()); //opcode
    //message.push_back('0');
    //message.push_back(std::to_string(opcode)[0]);
    message.insert(message.end(), filename.begin(), filename.end()); //filename
    message.push_back('\0');
    message.insert(message.end(), mode.begin(), mode.end()); //mode
    message.push_back('\0');
    if (blksize != -1)
    {
        std::string blksize_str = std::to_string(blksize);
        message.insert(message.end(), "blksize", "blksize" + 7);
        message.push_back('\0');
        message.insert(message.end(), blksize_str.begin(), blksize_str.end());
        message.push_back('\0');
    }
    if (timeout != -1)
    {
        std::string timeout_str = std::to_string(timeout);
        message.insert(message.end(), "timeout", "timeout" + 7);
        message.push_back('\0');
        message.insert(message.end(), timeout_str.begin(), timeout_str.end());
        message.push_back('\0');
    }
    if (tsize != -1)
    {
        std::string tsize_str = std::to_string(tsize);
        message.insert(message.end(), "tsize", "tsize" + 5);
        message.push_back('\0');
        message.insert(message.end(), tsize_str.begin(), tsize_str.end());
        message.push_back('\0');
    }
    if (sendto(udpSocket, message.data(), message.size(), 0, (struct sockaddr*)&destination, sizeof(destination)) == -1) {
        std::cout << "Chyba při odesílání broadcast zprávy: " << strerror(errno) << std::endl;
        close(udpSocket);
        return -1;
    }
    std::cout << "RRQ packet sent." << std::endl;
    return 0;
}

DATAPacket::DATAPacket(unsigned short blknum, std::vector<char> data)
{
    this->opcode = 3;
    this->blknum = blknum;
    this->data = data;
}

int DATAPacket::parse(std::string receivedMessage) {
    if (receivedMessage.length() < 4)
    {
        return -1;
    }

    opcode = getOpcode(receivedMessage);
    std::string block_number_str = receivedMessage.substr(2, 2);
    blknum = (static_cast<unsigned char>(block_number_str[0]) << 8) | static_cast<unsigned char>(block_number_str[1]);
    std::cout << "Received data with block number: " << blknum << std::endl;

    std::string data_string = receivedMessage.substr(4);
    std::vector<char> data_char(data_string.begin(), data_string.end());
    data = data_char;

    std::cout << "DATA packet parsed." << std::endl;
    return 0;
}

int DATAPacket::send(int udpSocket, sockaddr_in destination) const {
    std::vector<char> message;
    //message.push_back('0');
    //message.push_back(std::to_string(opcode)[0]);
    std::vector<char> opcode = intToBytes(this->opcode);
    message.insert(message.end(), opcode.begin(), opcode.end()); //opcode
    std::vector<char> block_number = intToBytes(blknum);
    message.insert(message.end(), block_number.begin(), block_number.end()); //blocknumber
    message.insert(message.end(), data.begin(), data.end()); //mode

    if (sendto(udpSocket, message.data(), message.size(), 0, (struct sockaddr*)&destination, sizeof(destination)) == -1) {
        std::cout << "Chyba při odesílání broadcast zprávy DATA packet: " << strerror(errno) << std::endl;
        close(udpSocket);
        return -1;
    }
    std::cout << "DATA packet sent." << std::endl;
    return 0;
}

ACKPacket::ACKPacket(unsigned short blknum)
{
    this->opcode = 4;
    this->blknum = blknum;
}

int ACKPacket::parse(std::string receivedMessage){
    if ((receivedMessage.length() != 4))
    {
        return -1;
    }
    opcode = getOpcode(receivedMessage);
    std::string block_number_str = receivedMessage.substr(2, 2);
    blknum = (static_cast<unsigned char>(block_number_str[0]) << 8) | static_cast<unsigned char>(block_number_str[1]);

    std::cout << "ACK packet parsed" << std::endl;
    return 0;
}

int ACKPacket::send(int udpSocket, sockaddr_in destination) const {
    std::vector<char> message;
    std::vector<char> opcode = intToBytes(this->opcode);
    message.insert(message.end(), opcode.begin(), opcode.end()); //opcode
    std::vector<char> block_number = intToBytes(blknum);
    message.insert(message.end(), block_number.begin(), block_number.end()); //blocknumber

    if (sendto(udpSocket, message.data(), message.size(), 0, (struct sockaddr*)&destination, sizeof(destination)) == -1) {
        std::cout << "Chyba při odesílání broadcast zprávy ACK packet: " << strerror(errno) << std::endl;
        close(udpSocket);
        return -1;
    }
    std::cout << "ACK packet sent." << std::endl;
    return 0;
}

OACKPacket::OACKPacket(unsigned short blknum, bool blocksize_set, int blocksize, int timeout, int tsize)
{
    this->opcode = 6;
    this->blknum = blknum;
    this->timeout = timeout;
    this->blksize = blocksize;
    this->tsize = tsize;
    this->blksize_set = blocksize_set;
}

int OACKPacket::parse(std::string receivedMessage) {
    std::cout << "OACKPacket parsing\n";
    if ((receivedMessage.length() < 2) || (receivedMessage[2] >= '0' && receivedMessage[2] <= '9' && receivedMessage[3] >= '0' && receivedMessage[3] <= '9'))
    {
        return -1;
    }

    opcode = getOpcode(receivedMessage);
    int optionIndex = 2;
    std::string option;
    while(optionIndex != -1 && optionIndex != (int)receivedMessage.length())
    {
        option = getSingleArgument(optionIndex, receivedMessage);
        if (optionIndex == -1)
        {
            //TODO error packet
            std::cout << "CHYBA" << std::endl;
            return -1;
        } else if (option == "blksize")
        {
            if (blksize == -1)
            {
                setOption(&blksize, &optionIndex, receivedMessage);
            } else {
                //TODO error packet
                std::cout << "CHYBA" << std::endl;
                return -1;
            }
        } else if (option == "timeout")
        {
            if (timeout == -1)
            {
                setOption(&timeout, &optionIndex, receivedMessage);
            } else {
                //TODO error packet
                std::cout << "CHYBA" << std::endl;
                return -1;
            }
        } else if (option == "tsize")
        {
            if (tsize == -1)
            {
                setOption(&tsize, &optionIndex, receivedMessage);
            } else {
                //TODO error packet
                std::cout << "CHYBA" << std::endl;
                return -1;
            }
        } else {
            //TODO error packet
            std::cout << "CHYBA" << std::endl;
            return -1;
        }
        optionIndex = getAnotherStartIndex(optionIndex, receivedMessage);

    }
    std::cout << "OACK packet parsed." << std::endl;
    return 0;
}

/*std::string OACKPacket::create(Session* session) const {
    std::cout << "RRQWRQPacket creating\n";
    std::string response = "06";
    if (session->blksize_set == true)
    {
        response += "blksize";
        response += '\0';
        response += std::to_string(session->blksize);
        response += '\0';
    }
    if (session->timeout != 1)
    {
        response += "timeout";
        response += '\0';
        response += std::to_string(session->timeout);
        response += '\0';
    }
    if (session->tsize != 1)
    {
        response += "tsize";
        response += '\0';
        response += std::to_string(session->tsize);
        response += '\0';
    }
    return response;
}*/

int OACKPacket::send(int udpSocket, sockaddr_in destination) const {
    std::vector<char> message;
    std::vector<char> opcode = intToBytes(this->opcode);
    message.insert(message.end(), opcode.begin(), opcode.end()); //opcode
    //message.push_back('0');
    //message.push_back(std::to_string(opcode)[0]);
    if (blksize_set == true)
    {
        std::string blksize_str = std::to_string(blksize);
        message.insert(message.end(), "blksize", "blksize" + 7);
        message.push_back('\0');
        message.insert(message.end(), blksize_str.begin(), blksize_str.end());
        message.push_back('\0');
    }
    if (timeout != -1)
    {
        std::string timeout_str = std::to_string(timeout);
        message.insert(message.end(), "timeout", "timeout" + 7);
        message.push_back('\0');
        message.insert(message.end(), timeout_str.begin(), timeout_str.end());
        message.push_back('\0');
    }
    if (tsize != -1)
    {
        std::string tsize_str = std::to_string(tsize);
        message.insert(message.end(), "tsize", "tsize" + 5);
        message.push_back('\0');
        message.insert(message.end(), tsize_str.begin(), tsize_str.end());
        message.push_back('\0');
    }

    if (sendto(udpSocket, message.data(), message.size(), 0, (struct sockaddr*)&destination, sizeof(destination)) == -1) {
        std::cout << "Chyba při odesílání broadcast zprávy OACK packet: " << strerror(errno) << std::endl;
        close(udpSocket);
        return -1;
    }
    std::cout << "OACK packet sent." << std::endl;
    return 0;
}

ERRORPacket::ERRORPacket(unsigned short error_code, std::string error_message)
{
    this->opcode = 5;
    this->error_code = error_code;
    this->error_message = error_message;
}


int ERRORPacket::parse(std::string receivedMessage){
    if ((receivedMessage.length() < 5))
    {
        return -1;
    }
    std::string error_code_str = receivedMessage.substr(2, 2);
    this->error_code = (static_cast<unsigned char>(error_code_str[0]) << 8) | static_cast<unsigned char>(error_code_str[1]); 
    this->error_message = receivedMessage.substr(4);

    std::cout << "ERROR packet parsed." << std::endl;
    return 0;
}

int ERRORPacket::send(int udpSocket, sockaddr_in destination) const {
    std::vector<char> message;
    std::vector<char> opcode = intToBytes(this->opcode);
    message.insert(message.end(), opcode.begin(), opcode.end()); //opcode
    //message.push_back('0');
    //message.push_back(std::to_string(opcode)[0]);
    std::vector<char> error_code = intToBytes(this->error_code);
    message.insert(message.end(), error_code.begin(), error_code.end()); //error code
    message.insert(message.end(), this->error_message.begin(), this->error_message.end());

    if (sendto(udpSocket, message.data(), message.size(), 0, (struct sockaddr*)&destination, sizeof(destination)) == -1) {
        std::cout << "Chyba při odesílání broadcast zprávy ERROR packet: " << strerror(errno) << std::endl;
        close(udpSocket);
        return -1;
    }
    std::cout << "ERROR packet sent." << std::endl;
    return 0;
}