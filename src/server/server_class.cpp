#include <iostream>
#include "../../include/server/server_class.hpp"

Server* Server::server_ = nullptr;;

/**
 * Static methods should be defined outside the class.
 */
Server *Server::getInstance()
{
    /**
     * This is a safer way to create an instance. instance = new Singleton is
     * dangeruous in case two instance threads wants to access at the same time
     */
    if(server_==nullptr){
        server_ = new Server();
        server_->port = 69;
        server_->root_dirpath = "";
    }
    return server_;
}

int Server::parse_arguments(int argc, char* argv[], Server *server)
{
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
                    return StatusCode::INVALID_ARGUMENTS;
                } else 
                {
                    server->port = std::atoi(optarg);
                    //server_cnfg.port = std::atoi(optarg);
                    if (server->port <= 0 || server->port > 65535) 
                    {
                        return StatusCode::INVALID_ARGUMENTS;
                    }
                    port_set = true;
                }
                break;
            case '?':
                return StatusCode::INVALID_ARGUMENTS;
            default:
                break;
        }
    }

    if (optind >= argc) {
        return StatusCode::INVALID_ARGUMENTS;
    }
    if ((argc > 2 && port_set == false) || argc > 4) {
        return StatusCode::INVALID_ARGUMENTS;
    }

    server->root_dirpath = argv[optind];

    std::cout << "Port: " << server->port << std::endl;
    std::cout << "Directory: " << server->root_dirpath << std::endl;

    return StatusCode::SUCCESS;
}

int Server::listen(Server *server)
{
    int udpSocket = initialize_connection(server);
    if (udpSocket == StatusCode::CONNECTION_ERROR || udpSocket == StatusCode::SOCKET_ERROR) {
        return udpSocket;
    }

    std::cout << "Listening... " << strerror(errno) << std::endl;
    server_loop(udpSocket);

    close(udpSocket);

    return StatusCode::SUCCESS;
}

int Server::create_udp_socket() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        std::cout << "Chyba při vytváření socketu: " << strerror(errno) << std::endl;
        return StatusCode::SOCKET_ERROR;
    }
    return udpSocket;
}

int Server::setup_udp_socket(int udpSocket, Server *server){
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server->port); // Port 69 pro TFTP
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Adresa "0.0.0.0"

    if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cout << "Chyba při bind: " << strerror(errno) << std::endl;
        close(udpSocket);
        return StatusCode::CONNECTION_ERROR;
    }
    return udpSocket;
}

int Server::initialize_connection(Server *server)
{
    int udpSocket = create_udp_socket();
    if (udpSocket == StatusCode::SOCKET_ERROR) {
        return StatusCode::SOCKET_ERROR;
    }
    udpSocket = setup_udp_socket(udpSocket, server);
    if(udpSocket == StatusCode::CONNECTION_ERROR){
        return StatusCode::CONNECTION_ERROR;
    }
    return udpSocket;

}

int Server::respond_to_client(int udpSocket, const char* message, size_t messageLength, struct sockaddr_in* clientAddr, socklen_t clientAddrLen) {
    if (sendto(udpSocket, message, messageLength, 0, (struct sockaddr*)clientAddr, clientAddrLen) == -1) {
        std::cout << "Chyba při odesílání odpovědi: " << strerror(errno) << std::endl;
        return StatusCode::CONNECTION_ERROR;
    }
    return StatusCode::SUCCESS;
}


void Server::server_loop(int udpSocket) {
    char buffer[1024]; // Buffer pro přijatou zprávu
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    while (true) {
        int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesRead == StatusCode::CONNECTION_ERROR) {
            std::cout << "Chyba při čekání na zprávu: " << strerror(errno) << std::endl;
            continue; // Pokračujeme ve smyčce i po chybě
        }
        std::string receivedMessage(buffer, bytesRead);
        std::cout << "Prijata zprava: " << receivedMessage << std::endl;
        std::cout << "Prijato bytu:" << bytesRead << std::endl;

        // Zde byste měli provádět operace s přijatou zprávou.
        // Například zde můžete zprávu analyzovat a zpracovat.

        // Odpověd můžete poslat zpět klientovi:
        respond_to_client(udpSocket, buffer, bytesRead, &clientAddr, clientAddrLen);
    }
}