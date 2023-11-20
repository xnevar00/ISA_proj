/**
 * @file tftp_packet_class.cpp
 * @author Veronika Nevarilova (xnevar00)
 * @date 11/2023
 */


#include "../../include/packet/tftp_packet_class.hpp"

std::pair<TFTPPacket *, int> TFTPPacket::parsePacket(std::string receivedMessage, std::string src_IP, int src_port, int dst_port)
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
            return {nullptr, -1};
            break;
    }

    int ok = packet->parse(receivedMessage);
    if(ok == -1)    // general error while parsing the packet
    {
        OutputHandler::getInstance()->print_to_cout("Error in packet parsing.");
        return {nullptr, -1};
    }
    if (ok == -2)   // error caused by options
    {
        OutputHandler::getInstance()->print_to_cout("Error in packet parsing.");
        return {nullptr, -2};
    }
        switch(opcode){
        case 1:
        case 2:
            printRrqWrqInfo(packet->opcode, src_IP, src_port, packet->filename, packet->mode, packet->options_string);
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
            printOackInfo(src_IP, src_port, packet->options_string);
            break;
        default :
            break;
    }
    return {packet, 0};
}

int TFTPPacket::sendAck(int block_number, int udp_socket, sockaddr_in addr, std::vector<char> *last_data)
{
    ACKPacket response_packet(block_number);
    if (response_packet.send(udp_socket, addr, last_data) == -1)
    {
        return -1;
    }
    return 0;
}

int TFTPPacket::receiveAck(int udp_socket, short unsigned block_number, int client_port)
{
    OutputHandler::getInstance()->print_to_cout("Waiting for ACK packet...");
    char received_data[MAXMESSAGESIZE];
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);

    int bytesRead = recvfrom(udp_socket, received_data, sizeof(received_data), 0, (struct sockaddr*)&addr, &addrLen);
    if (bytesRead == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // timeout
            return StatusCode::TIMEOUT;
        } else {
            OutputHandler::getInstance()->print_to_cout("Error in recvfrom: " + std::string(strerror(errno)));
            return StatusCode::PARSING_ERROR;
        }
    }

    if (getPort(addr) != client_port)
    {
        TFTPPacket::sendError(udp_socket, addr, 5, "Unknown transfer ID.");
        return StatusCode::UNK_TID;
    }

    std::string received_message(received_data, bytesRead);
    OutputHandler::getInstance()->print_to_cout("Received ACK packet.");
    if (bytesRead < 4) //4 bytes is the size of ACK packet
    {
        return StatusCode::PARSING_ERROR;
    }

    auto [packet, ok] = TFTPPacket::parsePacket(received_message, getIPAddress(addr), ntohs(addr.sin_port), getLocalPort(udp_socket));
    if (packet->opcode != Opcode::ACK)
    {
        if (packet->opcode == Opcode::ERROR)
        {
            return StatusCode::RECV_ERR;
        } else 
        {
            TFTPPacket::sendError(udp_socket, addr, 4, "Illegal TFTP operation.");
            return StatusCode::PARSING_ERROR;
        }
    }
    if ((ok == -1) || (packet->blknum != block_number))
    {
        return StatusCode::PARSING_ERROR;
    }

    return StatusCode::SUCCESS;
}

int TFTPPacket::receiveData(int udp_socket, int block_number, int block_size, std::ofstream *file, bool *last_packet, int client_port, bool *r_flag, std::string mode)
{
    char received_data[MAXMESSAGESIZE]; 
    struct sockaddr_in tmpClientAddr;
    socklen_t tmpClientAddrLen = sizeof(tmpClientAddr);

    OutputHandler::getInstance()->print_to_cout("Waiting for DATA packet...");
    int bytesRead = recvfrom(udp_socket, received_data, sizeof(received_data), 0, (struct sockaddr*)&tmpClientAddr, &tmpClientAddrLen);
    if (bytesRead == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // timeout
            return StatusCode::TIMEOUT;
        } else {
            OutputHandler::getInstance()->print_to_cout("Error in recvfrom: " + std::string(strerror(errno)));
            return StatusCode::PARSING_ERROR;
        }
    }
    if (getPort(tmpClientAddr) != client_port)
    {
        TFTPPacket::sendError(udp_socket, tmpClientAddr, 5, "Unknown transfer ID.");
        return StatusCode::UNK_TID;
    }

    std::string received_message(received_data, bytesRead);
    if (bytesRead < 4) //4 bytes is the size of DATA packet
    {
        TFTPPacket::sendError(udp_socket, tmpClientAddr, 4, "Illegal TFTP operation.");
        return StatusCode::PARSING_ERROR;
    }

    auto [packet, ok] = TFTPPacket::parsePacket(received_message, getIPAddress(tmpClientAddr), ntohs(tmpClientAddr.sin_port), getLocalPort(udp_socket));
    if (packet->opcode != Opcode::DATA)
    {
        if (packet->opcode == Opcode::ERROR)
        {
            return StatusCode::RECV_ERR;
        } else 
        {
            TFTPPacket::sendError(udp_socket, tmpClientAddr, 4, "Illegal TFTP operation.");
            return StatusCode::PARSING_ERROR;
        }
    }
    if (ok != 0 || (packet->opcode != Opcode::DATA && packet->opcode != Opcode::ERROR) || (packet->data.size() > (long unsigned)block_size))
    {
        TFTPPacket::sendError(udp_socket, tmpClientAddr, 4, "Illegal TFTP operation.");
        return StatusCode::PARSING_ERROR;
    }
    if (packet->blknum != block_number)
    {
        OutputHandler::getInstance()->print_to_cout("Wrong block number!");
        return StatusCode::PARSING_ERROR;
    }

    if ((unsigned short int)packet->data.size() < block_size)
    {
        // data shorter than block size = last packet
        *last_packet = true;
    }

    std::vector<char> transfered_data;
    if (mode == "netascii")
    {
        // handle '\r'
        for (size_t i = 0; i < packet->data.size(); i++) {
            if (*r_flag == true)
            {
                *r_flag = false;
                if (packet->data[i] == '\n') {
                    transfered_data.push_back('\n');
                    
                    continue;
                } else if (packet->data[i] == '\0') {
                    transfered_data.push_back('\r');
                    continue;
                }
            }
            if (packet->data[i] == '\r') {
                if (i == packet->data.size() - 1) {
                    *r_flag = true;
                    continue;
                } else if (packet->data[i + 1] == '\n') {
                    transfered_data.push_back('\n');
                    i++;
                    continue;
                } else if (packet->data[i + 1] == '\0') {
                    transfered_data.push_back('\r');
                    i++;
                    continue;
                }
            }
            transfered_data.push_back(packet->data[i]);
        }
    } else
    {
        transfered_data = packet->data;
    }
    
    // add the received data to the file
    file->write(transfered_data.data(), transfered_data.size());

    if (*last_packet == true)
    {
        file->close();
    }
    return StatusCode::SUCCESS;
}

int TFTPPacket::sendData(int udp_socket, sockaddr_in addr, int block_number, int block_size, int bytes_read, std::vector<char> data, bool *last_packet, std::vector<char> *last_data)
{
    if (bytes_read < 0) 
    {
        OutputHandler::getInstance()->print_to_cout("Error reading from stdin.");
        return -1;
    }

    data.resize(bytes_read);

    if (data.size() < (long unsigned)block_size)
    {
        *last_packet = true;
    }

    DATAPacket response_packet(block_number, data);
    if (response_packet.send(udp_socket, addr, last_data) == -1)
    {
        OutputHandler::getInstance()->print_to_cout("Error sending DATA packet.");
        return -1;
    }
    return 0;
}

int TFTPPacket::sendError(int udp_socket, sockaddr_in addr, int error_code, std::string error_message)
{
    ERRORPacket error_packet(error_code, error_message);
    if (error_packet.send(udp_socket, addr, nullptr) == -1)
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
    // if default, dont send as option
    if (blksize == 512)
    {
        this->blksize = -1;
    } else
    {
        this->blksize = blksize;
    }
    this->tsize = tsize;
}

int RRQWRQPacket::parse(std::string receivedMessage) {
    options_string = "";
    if (receivedMessage.length() < 4)
    {
        return -1;
    }
    opcode = getOpcode(receivedMessage);

    int filenameIndex = 2;
    filename = getSingleArgument(filenameIndex, receivedMessage);
    if (filename == "")
    {
        // could not parse filename
        OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (1)");
        return -1;
    }
    int modeIndex = getAnotherStartIndex(filenameIndex, receivedMessage);
    mode = getSingleArgument(modeIndex, receivedMessage);
    std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
    OutputHandler::getInstance()->print_to_cout("Mode: " + mode);
    if (mode != "netascii" && mode != "octet")
    {
        // wrong mode
        OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (2)");
        return -1;
    }

    int optionIndex = getAnotherStartIndex(modeIndex, receivedMessage);
    std::string option;
    while(optionIndex != -1 && optionIndex != (int)receivedMessage.length())
    {
        option = getSingleArgument(optionIndex, receivedMessage);
        std::transform(option.begin(), option.end(), option.begin(), ::tolower);
        if (option == "")
        {
            // could not parse option
            OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (3)");
            return -2;
        }
        if (optionIndex == -1)
        {
            OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (4)");
            return -2;
        } else if (option == "blksize")
        {
            if (blksize == -1)
            {
                options_string += " blksize";
                if (setOption(&blksize, &optionIndex, receivedMessage, &options_string) == -1)
                {
                    OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (5)");
                    return -2;
                }
            } else {
                OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (6)");
                return -2;
            }
        } else if (option == "timeout")
        {
            if (timeout == -1)
            {
                options_string += " timeout";
                if (setOption(&timeout, &optionIndex, receivedMessage, &options_string) == -1)
                {
                    OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (7)");
                    return -2;
                }
            } else {
                OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (8)");
                return -2;
            }
        } else if (option == "tsize")
        {
            if (tsize == -1)
            {
                options_string += " tsize";
                if(setOption(&tsize, &optionIndex, receivedMessage, &options_string) == -1)
                {
                    OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (9)");
                    return -2;
                }
            } else {
                OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (10)");
                return -2;
            }
        } else {
            //skipping unknown option         
            optionIndex = getAnotherStartIndex(optionIndex, receivedMessage);
            if (optionIndex == -1)
            {
                OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (11)");
                return -2;
            }
            if (getSingleArgument(optionIndex, receivedMessage) == "")
            {
                OutputHandler::getInstance()->print_to_cout("Error parsing RRQ/WRQ packet. (11)");
                return -2;
            }
            OutputHandler::getInstance()->print_to_cout("Ignoring an option since it is not supported.");
        }
        optionIndex = getAnotherStartIndex(optionIndex, receivedMessage);

    }
    OutputHandler::getInstance()->print_to_cout("RRQ/WRQ packet parsed.");
    return 0;
}

int RRQWRQPacket::send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const {
    std::vector<char> message;
    std::vector<char> opcode = intToBytes(this->opcode);
    message.insert(message.end(), opcode.begin(), opcode.end()); //opcode
    message.insert(message.end(), filename.begin(), filename.end()); //filename
    message.push_back('\0');
    message.insert(message.end(), mode.begin(), mode.end()); //mode
    message.push_back('\0');
    if (blksize != -1)  // if blksize set
    {
        std::string blksize_str = std::to_string(blksize);
        message.insert(message.end(), "blksize", "blksize" + 7);
        message.push_back('\0');
        message.insert(message.end(), blksize_str.begin(), blksize_str.end());
        message.push_back('\0');
    }
    if (timeout != -1)  // if timeout set
    {
        std::string timeout_str = std::to_string(timeout);
        message.insert(message.end(), "timeout", "timeout" + 7);
        message.push_back('\0');
        message.insert(message.end(), timeout_str.begin(), timeout_str.end());
        message.push_back('\0');
    }
    if (tsize != -1)    // if tsize set
    {
        std::string tsize_str = std::to_string(tsize);
        message.insert(message.end(), "tsize", "tsize" + 5);
        message.push_back('\0');
        message.insert(message.end(), tsize_str.begin(), tsize_str.end());
        message.push_back('\0');
    }
    if (sendto(udpSocket, message.data(), message.size(), 0, (struct sockaddr*)&destination, sizeof(destination)) == -1) {
        OutputHandler::getInstance()->print_to_cout("Chyba při odesílání broadcast zprávy: " + std::string(strerror(errno)));
        close(udpSocket);
        return -1;
    }
    *last_data = message;
    OutputHandler::getInstance()->print_to_cout("RRQ packet sent.");
    return 0;
}

DATAPacket::DATAPacket(unsigned short blknum, std::vector<char> data)
{
    this->opcode = 3;
    this->blknum = blknum;
    this->data = data;
}

int DATAPacket::parse(std::string receivedMessage) 
{
    if (receivedMessage.length() < 4)   //4 bytes is the size of DATA packet
    {
        return -1;
    }

    opcode = getOpcode(receivedMessage);
    std::string block_number_str = receivedMessage.substr(2, 2); // block number starts at index 2 and has length 2
    blknum = (static_cast<unsigned char>(block_number_str[0]) << 8) | static_cast<unsigned char>(block_number_str[1]);
    OutputHandler::getInstance()->print_to_cout("Received data with block number: " + std::to_string(blknum));

    std::string data_string = receivedMessage.substr(4);    // data starts at index 4
    std::vector<char> data_char(data_string.begin(), data_string.end());
    data = data_char;

    OutputHandler::getInstance()->print_to_cout("Data packet parsed.");
    return 0;
}

int DATAPacket::send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const {
    std::vector<char> message;
    std::vector<char> opcode = intToBytes(this->opcode);
    message.insert(message.end(), opcode.begin(), opcode.end()); //opcode
    std::vector<char> block_number = intToBytes(blknum);
    message.insert(message.end(), block_number.begin(), block_number.end()); //blocknumber
    message.insert(message.end(), data.begin(), data.end()); //mode

    if (sendto(udpSocket, message.data(), message.size(), 0, (struct sockaddr*)&destination, sizeof(destination)) == -1) {
        OutputHandler::getInstance()->print_to_cout("Error sending DATA packet: " + std::string(strerror(errno)));
        close(udpSocket);
        return -1;
    }
    *last_data = message;
    OutputHandler::getInstance()->print_to_cout("DATA packet sent.");
    return 0;
}

ACKPacket::ACKPacket(unsigned short blknum)
{
    this->opcode = 4;
    this->blknum = blknum;
}

int ACKPacket::parse(std::string receivedMessage){
    if ((receivedMessage.length() != 4))    //4 bytes is the size of ACK packet
    {
        return -1;
    }
    opcode = getOpcode(receivedMessage);
    std::string block_number_str = receivedMessage.substr(2, 2); // block number starts at index 2 and has length 2
    blknum = (static_cast<unsigned char>(block_number_str[0]) << 8) | static_cast<unsigned char>(block_number_str[1]);

    OutputHandler::getInstance()->print_to_cout("ACK packet parsed.");
    return 0;
}

int ACKPacket::send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const {
    std::vector<char> message;
    std::vector<char> opcode = intToBytes(this->opcode);
    message.insert(message.end(), opcode.begin(), opcode.end()); //opcode
    std::vector<char> block_number = intToBytes(blknum);
    message.insert(message.end(), block_number.begin(), block_number.end()); //blocknumber

    if (sendto(udpSocket, message.data(), message.size(), 0, (struct sockaddr*)&destination, sizeof(destination)) == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -3;
        } else {
            OutputHandler::getInstance()->print_to_cout("Error in recvfrom: " + std::string(strerror(errno)));
            close(udpSocket);
        }
    }
    *last_data = message;
    OutputHandler::getInstance()->print_to_cout("ACK packet sent.");
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
    OutputHandler::getInstance()->print_to_cout("OACKPacket parsing");
    if ((receivedMessage.length() < 2) || (receivedMessage[2] >= '0' && receivedMessage[2] <= '9' && receivedMessage[3] >= '0' && receivedMessage[3] <= '9'))
    {
        return -1;
    }

    opcode = getOpcode(receivedMessage);
    int optionIndex = 2;    //agreed options start at index 2
    std::string option;
    while(optionIndex != -1 && optionIndex != (int)receivedMessage.length())
    {
        option = getSingleArgument(optionIndex, receivedMessage);
        if (optionIndex == -1)
        {
            OutputHandler::getInstance()->print_to_cout("Error parsing OACK packet. (1)");
            return -1;
        } else if (option == "blksize")
        {
            options_string += " blksize";
            if (blksize == -1)
            {
                setOption(&blksize, &optionIndex, receivedMessage, &options_string);
            } else {
                OutputHandler::getInstance()->print_to_cout("Error parsing OACK packet. (2)");
                return -1;
            }
        } else if (option == "timeout")
        {
            options_string += " timeout";
            if (timeout == -1)
            {
                setOption(&timeout, &optionIndex, receivedMessage, &options_string);
            } else {
                OutputHandler::getInstance()->print_to_cout("Error parsing OACK packet. (3)");
                return -1;
            }
        } else if (option == "tsize")
        {
            options_string += " tsize";
            if (tsize == -1)
            {
                setOption(&tsize, &optionIndex, receivedMessage, &options_string);
            } else {
                OutputHandler::getInstance()->print_to_cout("Error parsing OACK packet. (4)");
                return -1;
            }
        } else {
            OutputHandler::getInstance()->print_to_cout("Error parsing OACK packet. (5)");
            return -1;
        }
        optionIndex = getAnotherStartIndex(optionIndex, receivedMessage);

    }
    OutputHandler::getInstance()->print_to_cout("OACK packet parsed.");
    return 0;
}

int OACKPacket::send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const {
    std::vector<char> message;
    std::vector<char> opcode = intToBytes(this->opcode);
    message.insert(message.end(), opcode.begin(), opcode.end()); //opcode

    if (blksize_set == true)    // if blksize set
    {
        std::string blksize_str = std::to_string(blksize);
        message.insert(message.end(), "blksize", "blksize" + 7);
        message.push_back('\0');
        message.insert(message.end(), blksize_str.begin(), blksize_str.end());
        message.push_back('\0');
    }
    if (timeout != -1)  // if timeout set
    {
        std::string timeout_str = std::to_string(timeout);
        message.insert(message.end(), "timeout", "timeout" + 7);
        message.push_back('\0');
        message.insert(message.end(), timeout_str.begin(), timeout_str.end());
        message.push_back('\0');
    }
    if (tsize != -1) // if tsize set
    {
        std::string tsize_str = std::to_string(tsize);
        message.insert(message.end(), "tsize", "tsize" + 5);
        message.push_back('\0');
        message.insert(message.end(), tsize_str.begin(), tsize_str.end());
        message.push_back('\0');
    }

    if (sendto(udpSocket, message.data(), message.size(), 0, (struct sockaddr*)&destination, sizeof(destination)) == -1) {
        OutputHandler::getInstance()->print_to_cout("Error sending OACK packet: " + std::string(strerror(errno)));
        close(udpSocket);
        return -1;
    }
    *last_data = message;
    OutputHandler::getInstance()->print_to_cout("OACK packet sent.");
    return 0;
}

ERRORPacket::ERRORPacket(unsigned short error_code, std::string error_message)
{
    this->opcode = 5;
    this->error_code = error_code;
    this->error_message = error_message;
}


int ERRORPacket::parse(std::string receivedMessage){
    if ((receivedMessage.length() < 5)) //5 bytes is the minimum size of ERROR packet
    {
        return -1;
    }
    opcode = Opcode::ERROR;
    std::string error_code_str = receivedMessage.substr(2, 2);
    this->error_code = (static_cast<unsigned char>(error_code_str[0]) << 8) | static_cast<unsigned char>(error_code_str[1]); 
    this->error_message = receivedMessage.substr(4);

    OutputHandler::getInstance()->print_to_cout("ERROR packet parsed.");
    return 0;
}

int ERRORPacket::send(int udpSocket, sockaddr_in destination, std::vector<char> *last_data) const {
    std::vector<char> message;
    std::vector<char> opcode = intToBytes(this->opcode);
    message.insert(message.end(), opcode.begin(), opcode.end()); //opcode
    std::vector<char> error_code = intToBytes(this->error_code);
    message.insert(message.end(), error_code.begin(), error_code.end()); //error code
    message.insert(message.end(), this->error_message.begin(), this->error_message.end());

    if (sendto(udpSocket, message.data(), message.size(), 0, (struct sockaddr*)&destination, sizeof(destination)) == -1) {
        OutputHandler::getInstance()->print_to_cout("Error sending ERROR packet: " + std::string(strerror(errno)));
        close(udpSocket);
        return -1;
    }
    if (last_data != nullptr)
    {
        *last_data = message;
    }
    OutputHandler::getInstance()->print_to_cout("ERROR packet sent.");
    return 0;
}