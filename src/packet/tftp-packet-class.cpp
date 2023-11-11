#include "../../include/packet/tftp-packet-class.hpp"
#include <iostream>

void RRQPacket::parse(Session* session) const {
    std::cout << "RRQPacket parsing " << session->clientIP << session->clientPort << std::endl;
    
}

void WRQPacket::parse(Session* session) const {
    // implementace
    std::cout << "WRQPacket parsing\n";
}

void DATAPacket::parse(Session* session) const {
    // implementace
    std::cout << "DATAPacket parsing\n";
}

void ACKPacket::parse(Session* session) const {
    // implementace
    std::cout << "ACKPacket parsing\n";
}

void ERRORPacket::parse(Session* session) const {
    // implementace
    std::cout << "ERRORPacket parsing\n";
}