#include <iostream>
#include "../../include/server/server_class.hpp"
#include "../../include/packet/tftp-packet-class.hpp"

Server* Server::server_ = nullptr;;

Server *Server::getInstance()
{
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

void ClientHandler::handleClient(std::string receivedMessage, int bytesRead){
    if(bytesRead >= 2)
    {
        int opcode = getOpcode(receivedMessage);
        if (opcode != StatusCode::PACKET_ERROR)
        {  
            TFTPPacket *packet;
            switch (opcode){
                case Opcode::RRQ:
                    packet = new RRQPacket();
                    std::cout << "RRQ packet" << std::endl;
                    break;
                case Opcode::WRQ:
                    packet = new WRQPacket();
                    std::cout << "RRQ packet" << std::endl;
                    break;
                default:
                    //TODO error packet
                    break;
            }
        } else {
            std::cout << "Invalid packet" << std::endl;
            //TODO: poslat zpet error packet na zahajeni komunikace se bere jen rrq a wrq
        }
    }
    return;
}

void Server::server_loop(int udpSocket) {
    char buffer[65507]; // Buffer pro přijatou zprávu, max velikost UDP packetu
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    while (true) {
        int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesRead == StatusCode::CONNECTION_ERROR) {
            std::cout << "Chyba při čekání na zprávu: " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "New client!" << std::endl;
        ClientHandler clientHandlerObj;
        std::string receivedMessage(buffer, bytesRead);
        std::thread clientThread(&ClientHandler::handleClient, &clientHandlerObj, std::ref(receivedMessage), std::ref(bytesRead));
        clientThreads.push_back(std::move(clientThread));
    }
}

int ClientHandler::getOpcode(std::string receivedMessage)
{
    int opcode = StatusCode::PACKET_ERROR;
    if(receivedMessage[0] >= '0' && receivedMessage[0] <= '9' && receivedMessage[1] >= '0' && receivedMessage[1] <= '9')
    {
        std::string opcodeStr = receivedMessage.substr(0, 2);
        opcode = std::stoi(opcodeStr);
    } else{
        return StatusCode::PACKET_ERROR;
    }
    std::cout << "Opcode: " << opcode << std::endl;
    return opcode;
}