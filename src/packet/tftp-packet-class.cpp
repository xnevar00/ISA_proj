#include "../../include/packet/tftp-packet-class.hpp"
#include <iostream>
#include <string>
#include <cstring>

void RRQWRQPacket::parse(Session* session, std::string receivedMessage) const {

    int filenameIndex = 2;
    session->filename = getSingleArgument(filenameIndex, receivedMessage);
    if (session->filename == "")
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return;
    }
    int modeIndex = getAnotherStartIndex(filenameIndex, receivedMessage);
    session->mode = getSingleArgument(modeIndex, receivedMessage);
    if (session->mode == "")
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return;
    }
    int optionIndex = getAnotherStartIndex(modeIndex, receivedMessage);
    //std::cout << "Option index: " << optionIndex << std::endl;
    if (optionIndex == -1 || optionIndex == (int)receivedMessage.length())
    {
        return;
    }
    optionIndex = getPairArgument(optionIndex, receivedMessage, session);
    if (optionIndex == -1)
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return;
    }
    optionIndex = getAnotherStartIndex(optionIndex, receivedMessage);
    if (optionIndex == -1 || optionIndex == (int)receivedMessage.length())
    {
        std::cout << "No other options" << std::endl;
        return;
    }
    optionIndex = getPairArgument(optionIndex, receivedMessage, session);
    if (optionIndex == -1)
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return;
    }
    
}

void DATAPacket::parse(Session* session, std::string receivedMessage) const {
    // implementace
    std::cout << "DATAPacket parsing\n";
}

void ACKPacket::parse(Session* session, std::string receivedMessage) const {
    // implementace
    std::cout << "ACKPacket parsing\n";
}

void ERRORPacket::parse(Session* session, std::string receivedMessage) const {
    // implementace
    std::cout << "ERRORPacket parsing\n";
}