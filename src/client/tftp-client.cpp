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
        std::cerr << "Chyba při vytváření socketu: " << strerror(errno) << std::endl;
    }
    return udpSocket;
}

int receive_respond_from_server(int udpSocket) {
    struct sockaddr_in serverAddr;
    socklen_t serverAddrLen = sizeof(serverAddr);
    char buffer[1024]; // Buffer pro přijatou odpověď

    int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&serverAddr, &serverAddrLen);
    if (bytesRead == -1) {
        std::cerr << "Chyba při přijímání odpovědi: " << strerror(errno) << std::endl;
        return -1;
    }

    std::string receivedResponse(buffer, bytesRead);
    std::cerr << "Přijata odpověď od serveru: " << receivedResponse << std::endl;

    // Zde můžete provádět operace s přijatou odpovědí od serveru.
    // Například zpracovat obsah odpovědi nebo provést další akce.

    return 0;
}

int send_broadcast_message(int udpSocket, Client *client) {
    struct sockaddr_in broadcastAddr;
    std::memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(client->port); // Port 69 pro TFTP
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Broadcast adresa - pozdeji zmenit na INADDR_BROADCAST?

    const char* message = "ahoj2";
    if (sendto(udpSocket, message, strlen(message), 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) == -1) {
        std::cerr << "Chyba při odesílání broadcast zprávy: " << strerror(errno) << std::endl;
        close(udpSocket);
        return -1;
    }
    return 0;
}

int communicate(Client *client)
{
    int udpSocket = create_udp_socket();
    if (udpSocket == -1) {
        return udpSocket;
    }

    if (send_broadcast_message(udpSocket, client) == -1) {
        close(udpSocket);
        return -1;
    }

    std::cerr << "Broadcast message sent." << std::endl;

    if (receive_respond_from_server(udpSocket) == -1) {
    close(udpSocket);
    return -1;
    }

    // Nezapomeňte uzavřít socket, když skončíte s používáním.
    close(udpSocket);

    return 0;
}

int main(int argc, char* argv[]) {
    Client *client = Client::getInstance();
    int ok = client->parse_arguments(argc, argv, client);
    if (ok == -1)
    {
        std::cerr << "Chyba v zadani argumentu" << std::endl;
        return -1;
    } else if (ok == 0) {
        std::cout << "Všechno je v pořádku" << std::endl;
    }
    ok = communicate(client);
    if (ok == -1)
    {
        return -1;
    }
    return 0;
}