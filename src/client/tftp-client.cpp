#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../../include/client/client_class.hpp"

int create_udp_socket() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        std::cout << "Chyba při vytváření socketu: " << strerror(errno) << std::endl;
        return -1;
    }

    
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(0);
    

    if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cout << "Chyba při bind: " << strerror(errno) << std::endl;
        close(udpSocket);
        return -1;
    }

    struct sockaddr_in localAddress;
    socklen_t addressLength = sizeof(localAddress);
    getsockname(udpSocket, (struct sockaddr*)&localAddress, &addressLength);


    int port = ntohs(localAddress.sin_port);

    std::cout << "Klientský socket běží na portu: " << port << std::endl;

    return udpSocket;
}

int receive_respond_from_server(int udpSocket) {
    struct sockaddr_in serverAddr;
    socklen_t serverAddrLen = sizeof(serverAddr);
    char buffer[1024]; // Buffer pro přijatou odpověď

    int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&serverAddr, &serverAddrLen);
    if (bytesRead == -1) {
        std::cout << "Chyba při přijímání odpovědi: " << strerror(errno) << std::endl;
        return StatusCode::CONNECTION_ERROR;
    }

    std::string receivedResponse(buffer, bytesRead);
    std::cout << "Přijata odpověď od serveru: " << receivedResponse << std::endl;

    // Zde můžete provádět operace s přijatou odpovědí od serveru.
    // Například zpracovat obsah odpovědi nebo provést další akce.

    return StatusCode::SUCCESS;
}

int send_broadcast_message(int udpSocket, Client *client) {
    struct sockaddr_in broadcastAddr;
    std::memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(client->port); // Port 69 pro TFTP
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Broadcast adresa - pozdeji zmenit na INADDR_BROADCAST?

    const char* message = "02ahoj\0mode\0timeout\0""5\0blksize\0""512\0";
    if (sendto(udpSocket, message, 34, 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) == -1) {
        std::cout << "Chyba při odesílání broadcast zprávy: " << strerror(errno) << std::endl;
        close(udpSocket);
        return StatusCode::CONNECTION_ERROR;
    }
    return StatusCode::SUCCESS;
}

int communicate(Client *client)
{
    int udpSocket = create_udp_socket();
    if (udpSocket == -1) {
        return StatusCode::SOCKET_ERROR;
    }

    if (send_broadcast_message(udpSocket, client) == StatusCode::CONNECTION_ERROR) {
        close(udpSocket);
        return StatusCode::CONNECTION_ERROR;
    }

    std::cout << "Broadcast message sent." << std::endl;

    if (receive_respond_from_server(udpSocket) == StatusCode::CONNECTION_ERROR) {
        close(udpSocket);
        return StatusCode::CONNECTION_ERROR;
    }

    close(udpSocket);

    return StatusCode::SUCCESS;
}

int main(int argc, char* argv[]) {
    Client *client = Client::getInstance();

    int ok = client->parse_arguments(argc, argv, client);
    if (ok != StatusCode::SUCCESS)
    {
        std::cout << "Chyba v zadani argumentu" << std::endl;
        return StatusCode::INVALID_ARGUMENTS;
    } else {
        std::cout << "Všechno je v pořádku" << std::endl;
    }
    ok = communicate(client);
    return ok;
}