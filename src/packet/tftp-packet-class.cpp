#include "../../include/packet/tftp-packet-class.hpp"
#include <iostream>

void RRQPacket::parse() const {
    // implementace
    std::cout << "RRQPacket parsing\n";
}

void WRQPacket::parse() const {
    // implementace
    std::cout << "WRQPacket parsing\n";
}

void DATAPacket::parse() const {
    // implementace
    std::cout << "DATAPacket parsing\n";
}

void ACKPacket::parse() const {
    // implementace
    std::cout << "ACKPacket parsing\n";
}

void ERRORPacket::parse() const {
    // implementace
    std::cout << "ERRORPacket parsing\n";
}