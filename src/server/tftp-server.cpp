#include <iostream>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../../include/server/server_class.hpp"

int create_udp_socket() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        std::cerr << "Chyba při vytváření socketu: " << strerror(errno) << std::endl;
        return -1;
    }
    return udpSocket;
}

int setup_udp_socket(int udpSocket, Server *server){
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server->port); // Port 69 pro TFTP
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Adresa "0.0.0.0"

    if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Chyba při bind: " << strerror(errno) << std::endl;
        close(udpSocket);
        return -1;
    }
    return udpSocket;
}

int initialize_connection(Server *server)
{
    int udpSocket = create_udp_socket();
    if (udpSocket == -1) {
        return udpSocket;
    }
    udpSocket = setup_udp_socket(udpSocket, server);
    return udpSocket;

}

int respond_to_client(int udpSocket, const char* message, size_t messageLength, struct sockaddr_in* clientAddr, socklen_t clientAddrLen) {
    if (sendto(udpSocket, message, messageLength, 0, (struct sockaddr*)clientAddr, clientAddrLen) == -1) {
        std::cerr << "Chyba při odesílání odpovědi: " << strerror(errno) << std::endl;
        return -1;
    }
    return 0;
}


void server_loop(int udpSocket) {
    char buffer[1024]; // Buffer pro přijatou zprávu
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    while (true) {
        int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesRead == -1) {
            std::cerr << "Chyba při čekání na zprávu: " << strerror(errno) << std::endl;
            continue; // Pokračujeme ve smyčce i po chybě
        }
        std::string receivedMessage(buffer, bytesRead);
        std::cerr << "Prijata zprava: " << receivedMessage << std::endl;
        std::cerr << "Prijato bytu:" << bytesRead << std::endl;

        // Zde byste měli provádět operace s přijatou zprávou.
        // Například zde můžete zprávu analyzovat a zpracovat.

        // Odpověd můžete poslat zpět klientovi:
        respond_to_client(udpSocket, buffer, bytesRead, &clientAddr, clientAddrLen);
    }
}

int listen(Server *server)
{
    int udpSocket = initialize_connection(server);
    if (udpSocket == -1) {
        return udpSocket;
    }

    // Zde můžete provádět operace s otevřeným socketem na portu 69 (TFTP).
    std::cerr << "Listening... " << strerror(errno) << std::endl;

    server_loop(udpSocket);
    // Nezapomeňte uzavřít socket, když skončíte s používáním.
    close(udpSocket);

    return 0;
}

int main(int argc, char* argv[]) {
    Server* server = Server::getInstance();

    int ok = server->parse_arguments(argc, argv, server);
    if (ok == -1)
    {
        std::cerr << "Chyba v zadani argumentu" << std::endl;
        return -1;
    } else if (ok == 0) {
        std::cout << "Všechno je v pořádku" << std::endl;
    }
    ok = listen(server);
    if (ok == -1)
    {
        std::cerr << "Chyba pri inicializaci socketu" << std::endl;
        return -1;
    }
    return 0;
}