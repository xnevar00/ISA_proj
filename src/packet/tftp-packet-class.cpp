#include "../../include/packet/tftp-packet-class.hpp"
#include <iostream>
#include <string>
#include <cstring>

int RRQWRQPacket::parse(Session* session, std::string receivedMessage) const {
    int filenameIndex = 2;
    session->filename = getSingleArgument(filenameIndex, receivedMessage);
    if (session->filename == "")
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return -1;
    }
    int modeIndex = getAnotherStartIndex(filenameIndex, receivedMessage);
    std::string mode = getSingleArgument(modeIndex, receivedMessage);
    if (mode == "")
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return -1;
    }
    setMode(mode, session);
    int optionIndex = getAnotherStartIndex(modeIndex, receivedMessage);
    //std::cout << "Option index: " << optionIndex << std::endl;
    if (optionIndex == -1 || optionIndex == (int)receivedMessage.length())
    {
        return 0;
    }
    optionIndex = getPairArgument(optionIndex, receivedMessage, session);
    if (optionIndex == -1)
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return -1;
    }
    optionIndex = getAnotherStartIndex(optionIndex, receivedMessage);
    if (optionIndex == -1 || optionIndex == (int)receivedMessage.length())
    {
        std::cout << "No other options" << std::endl;
        return 0;
    }
    optionIndex = getPairArgument(optionIndex, receivedMessage, session);
    if (optionIndex == -1)
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return -1;
    }
    return 0;
}

std::string RRQWRQPacket::create(Session* session) const {
    return "";
}

int DATAPacket::parse(Session* session, std::string receivedMessage) const {
    if(receivedMessage[2] >= '0' && receivedMessage[2] <= '9' && receivedMessage[3] >= '0' && receivedMessage[3] <= '9')
    {
        std::string block_number_str = receivedMessage.substr(2, 2);
        int block_number_int = std::stoi(block_number_str);
        if (block_number_int != session->block_number + 1)
        {
            return -1;
        } else
        {
            session->block_number = block_number_int;
            std::string data = receivedMessage.substr(4);
            std::cout << "DATA: " << data << std::endl;
            if ((int)(data.length()) < session->blksize)
            {
                session->last_packet = true;
            }
        }
    } else {
        return -1;
    }
    return 0;
}

std::string DATAPacket::create(Session* session) const {
    std::cout << "DATAPacket creating\n";
    return "";
}

int ACKPacket::parse(Session* session, std::string receivedMessage) const {
    // implementace
    std::cout << "ACKPacket parsing\n";
    return 0;
}

std::string ACKPacket::create(Session* session) const {
    std::cout << "ACKPacket creating\n";
    return "";
}

int OACKPacket::parse(Session* session, std::string receivedMessage) const {
    // implementace
    std::cout << "ACKPacket parsing\n";
    return 0;
}

std::string OACKPacket::create(Session* session) const {
    std::cout << "RRQWRQPacket creating\n";
    std::string response = "06";
    if (session->blksize_option == true)
    {
        response += "blksize";
        response += '\0';
        response += std::to_string(session->blksize);
        response += '\0';
    }
    if (session->timeout_option == true)
    {
        response += "timeout";
        response += '\0';
        response += std::to_string(session->timeout);
        response += '\0';
    }
    if (session->tsize_option == true)
    {
        response += "tsize";
        response += '\0';
        response += std::to_string(session->tsize);
        response += '\0';
    }
    return response;
}

int ERRORPacket::parse(Session* session, std::string receivedMessage) const {
    // implementace
    std::cout << "ERRORPacket parsing\n";
    return 0;
}

std::string ERRORPacket::create(Session* session) const {
    std::cout << "ERRORPacket creating\n";
    return "";
}