#include <iostream>
#include "../../include/server/server_class.hpp"
#include <arpa/inet.h>  // pro inet_ntoa

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
        std::cout << "Just received message: " << receivedMessage << std::endl;
        std::thread clientThread(&ClientHandler::handleClient, &clientHandlerObj, receivedMessage, bytesRead, clientAddr, clientAddrLen, root_dirpath);
        clientThreads.push_back(std::move(clientThread));
    }
}



















void ClientHandler::handleClient(std::string receivedMessage, int bytesRead, sockaddr_in clientAddr, socklen_t clientAddrLen, std::string root_dirpath){
    session.clientAddr = clientAddr;
    session.root_dirpath = root_dirpath;

    if(bytesRead >= 2)
    {
        int opcode = getOpcode(receivedMessage);
        if (opcode != StatusCode::PACKET_ERROR)
        {  
            TFTPPacket *packet;
            switch (opcode){
                case Opcode::RRQ:
                    packet = new RRQWRQPacket();
                    session.direction = Direction::Download;
                    std::cout << "RRQ packet" << std::endl;
                    handlePacket(packet, receivedMessage);
                    break;
                case Opcode::WRQ:
                    packet = new RRQWRQPacket();
                    session.direction = Direction::Upload;
                    std::cout << "WRQ packet" << std::endl;
                    handlePacket(packet, receivedMessage);
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
    return opcode;
}


void ClientHandler::handlePacket(TFTPPacket *packet, std::string receivedMessage)
{
    int ok = packet->parse(&session, receivedMessage);
    if (ok == -1) {
        //TODO error packet
        return;
    }
    if (session.direction == Direction::Upload)
    {
        if (setupFileForUpload(&session) == -1)
        {
            //TODO error packet
            return;
        }
    }
    if (createUdpSocket() == -1)
    {
        std::cout << "Chyba při vytvareni socketu: " << strerror(errno) << std::endl;
        return;
    }
    std::string response = generateResponse(packet);
    int messageLength = response.length();

    if (sendto(session.udpSocket, response.c_str(), response.length(), 0, (struct sockaddr*)&(session.clientAddr), sizeof(session.clientAddr)) == -1)
    {
        std::cout << "Chyba při odesílání odpovědi: " << strerror(errno) << std::endl;
        return;
    }
    std::cout << "Just sent message: " << response << std::endl;
    delete packet;

}

int ClientHandler::createUdpSocket()
{
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        return -1;
    }

    session.udpSocket = udpSocket;

    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(0);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // bind
    if (bind(session.udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cout << "Chyba při bindování socketu: " << strerror(errno) << std::endl;
        close(udpSocket);
        return -1;
    }

    struct sockaddr_in localAddress;
    socklen_t addressLength = sizeof(localAddress);
    getsockname(udpSocket, (struct sockaddr*)&localAddress, &addressLength);


    int port = ntohs(localAddress.sin_port);

    std::cout << "Serverovský socket běží na portu: " << port << std::endl;

    return 0;
}

std::string ClientHandler::generateResponse(TFTPPacket *packet)
{
    std::string response = "";  
    TFTPPacket *response_packet;
    if (typeid(*packet) == typeid(RRQWRQPacket)) {
        if (session.blksize_option || session.timeout_option || session.tsize_option)
        {
            response_packet = new OACKPacket();
            response = response_packet->create(&session);
        } else
        {
            response_packet = new ACKPacket();
            response = response_packet->create(&session);
        }
    }
    std::cout << "Response: " << response << std::endl;

    return response;
}

int ClientHandler::receiveData()
{
    char received_data[65507]; // Buffer pro přijatou zprávu, max velikost UDP packetu
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    int bytesRead = recvfrom(session.udpSocket, received_data, sizeof(received_data), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (bytesRead == StatusCode::CONNECTION_ERROR) {
        std::cout << "Chyba při čekání na zprávu: " << strerror(errno) << std::endl;
        return -1;
    }

    std::string received_message(received_data, bytesRead);
    if (bytesRead < 4)
    {
        //TODO error packet
        return -1;
    }
    int opcode = getOpcode(received_message);
    if (opcode != Opcode::DATA)
    {
        //TODO error packet
        return -1;
    } else
    {
        TFTPPacket *packet = new DATAPacket();
        if (packet->parse(&session, received_message) == -1)
        {
            //TODO error packet
            return -1;
        }
    }
    return 0;
}

int ClientHandler::transferFile()
{
    while(session.last_packet == false)
    {
        switch(current_state){
            case ClientHandlerState::WaitForTransfer:
                if(session.direction == Direction::Download)
                {
                    current_state = ClientHandlerState::SendData;
                } else if (session.direction == Direction::Upload)
                {
                    current_state = ClientHandlerState::ReceiveData;
                }
                break;
            case ClientHandlerState::SendAck:
                //send data
                current_state = ClientHandlerState::ReceiveData;
                break;
            case ClientHandlerState::ReceiveData:
                //receive data
                current_state = ClientHandlerState::SendAck;
                break;
            case ClientHandlerState::SendData:
                //send data
                current_state = ClientHandlerState::ReceiveAck;
                break;
            case ClientHandlerState::ReceiveAck:
                //receive ack
                current_state = ClientHandlerState::SendData;
                break;
            case ClientHandlerState::SendError:
                //send error
                break;
        }
    }
    return 0;
}