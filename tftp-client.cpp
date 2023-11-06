#include <iostream>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

struct client_config {
    int port = 69;
    std::string hostname = "";
    std::string filepath = "";
    std::string destFilepath = "";
    bool ok = true;
};

int create_udp_socket() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        std::cerr << "Chyba při vytváření socketu: " << strerror(errno) << std::endl;
    }
    return udpSocket;
}

int setup_udp_socket(int udpSocket, client_config client_cnfg){
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(client_cnfg.port); // Port 69 pro TFTP
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Adresa "0.0.0.0"

    // Nastavení socketu na komunikaci s konkrétním serverem:
    // serverAddr.sin_addr.s_addr = inet_addr("adresa_serveru");

    if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Chyba při bind: " << strerror(errno) << std::endl;
        close(udpSocket);
        return -1;
    }
    return udpSocket;
}

int initialize_connection(client_config client_cnfg)
{
    int udpSocket = create_udp_socket();
    if (udpSocket == -1) {
        return udpSocket;
    }
    udpSocket = setup_udp_socket(udpSocket, client_cnfg);
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

int send_broadcast_message(int udpSocket, client_config client_cnfg) {
    struct sockaddr_in broadcastAddr;
    std::memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(client_cnfg.port); // Port 69 pro TFTP
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Broadcast adresa - pozdeji zmenit na INADDR_BROADCAST?

    const char* message = "ahoj2";
    if (sendto(udpSocket, message, strlen(message), 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) == -1) {
        std::cerr << "Chyba při odesílání broadcast zprávy: " << strerror(errno) << std::endl;
        close(udpSocket);
        return -1;
    }
    return 0;
}

int communicate(client_config client_cnfg)
{
    int udpSocket = create_udp_socket();
    if (udpSocket == -1) {
        return udpSocket;
    }

    if (send_broadcast_message(udpSocket, client_cnfg) == -1) {
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

client_config wrong_arguments() {
    client_config error;
    error.ok = false;
    std::cerr << "Wrong arguments passed! Usage: tftp-server [-p port] directory" << std::endl;
    return error;
}

bool str_is_digits_only(std::string str) {
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

client_config parse_arguments(int argc, char* argv[])
{
    client_config client_cnfg;

    const struct option long_options[] = {
        {"hostname", required_argument, 0, 'h'},
        {"port", optional_argument, 0, 'p'},
        {"filepath", optional_argument, 0, 'f'},
        {"dest_filepath", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    int option;
    while ((option = getopt_long(argc, argv, "h:p:f:t:", long_options, nullptr)) != -1) {
        switch (option) {
            case 'h':
                client_cnfg.hostname = optarg;
                break;
            case 'p':
                if (str_is_digits_only(optarg) == false) 
                {
                    return wrong_arguments();
                } else 
                {
                    client_cnfg.port = std::atoi(optarg);
                    if (client_cnfg.port <= 0 || client_cnfg.port > 65535) 
                    {
                        return wrong_arguments();
                    }
                }
                break;
            case 'f':
                client_cnfg.filepath = optarg;
                break;
            case 't':
                client_cnfg.destFilepath = optarg;
                break;
            case '?':
                return wrong_arguments();
            default:
                break;
        }
    }

    if (client_cnfg.hostname.empty() || client_cnfg.destFilepath.empty()) {
        return wrong_arguments();
    }
    if ((argc < 5) || (argc > 9) || (argc%2 != 1)) {
        return wrong_arguments();
    }
    std::cout << "Hostname: " << client_cnfg.hostname << std::endl;
    std::cout << "Port: " << client_cnfg.port << std::endl;
    std::cout << "Filepath: " << client_cnfg.filepath << std::endl;
    std::cout << "Dest Filepath: " << client_cnfg.destFilepath << std::endl;

    return client_cnfg;
}

int main(int argc, char* argv[]) {
    client_config client_cnfg = parse_arguments(argc, argv);
    if (client_cnfg.ok == false) {
        return -1;
    } else
    {
        std::cout << "Všechno je v pořádku" << std::endl;
    }
    int ok = communicate(client_cnfg);
    if (ok == -1)
    {
        return -1;
    }
    return 0;
}