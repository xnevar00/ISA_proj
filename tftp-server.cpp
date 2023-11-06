#include <iostream>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

struct server_config {
    int port = 69;
    std::string root_dirpath = "";
    bool ok = true;
};

int create_udp_socket() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        std::cerr << "Chyba při vytváření socketu: " << strerror(errno) << std::endl;
        return -1;
    }
    return udpSocket;
}

int setup_udp_socket(int udpSocket, server_config server_cnfg){
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server_cnfg.port); // Port 69 pro TFTP
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

int initialize_connection(server_config server_cnfg)
{
    int udpSocket = create_udp_socket();
    if (udpSocket == -1) {
        return udpSocket;
    }
    udpSocket = setup_udp_socket(udpSocket, server_cnfg);
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

int listen(server_config server_cnfg)
{
    int udpSocket = initialize_connection(server_cnfg);
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

server_config wrong_arguments() {
    server_config error;
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

server_config parse_arguments(int argc, char* argv[])
{
    server_config server_cnfg;
    bool port_set = false;

    const struct option long_options[] = {
        {"port", required_argument, nullptr, 'p'},
        {0, 0, 0, 0}
    };

    int option;
    while ((option = getopt_long(argc, argv, "p:", long_options, nullptr)) != -1) 
    {
        switch (option) 
        {
            case 'p':
                if (str_is_digits_only(optarg) == false) 
                {
                    return wrong_arguments();
                } else 
                {
                    server_cnfg.port = std::atoi(optarg);
                    if (server_cnfg.port <= 0 || server_cnfg.port > 65535) 
                    {
                        return wrong_arguments();
                    }
                    port_set = true;
                }
                break;
            case '?':
                return wrong_arguments();
            default:
                break;
        }
    }

    if (optind >= argc) {
        return wrong_arguments();
    }
    if ((argc > 2 && port_set == false) || argc > 4) {
        return wrong_arguments();
    }

    server_cnfg.root_dirpath = argv[optind];

    std::cout << "Port: " << server_cnfg.port << std::endl;
    std::cout << "Directory: " << server_cnfg.root_dirpath << std::endl;

    return server_cnfg;
}



int main(int argc, char* argv[]) {
    server_config server_cnfg = parse_arguments(argc, argv);
    if (server_cnfg.ok == false) {
        return -1;
    } else
    {
        std::cout << "Všechno je v pořádku" << std::endl;
    }
    int ok = listen(server_cnfg);
    if (ok == -1)
    {
        std::cerr << "Chyba pri inicializaci socketu" << std::endl;
        return -1;
    }
    return 0;
}