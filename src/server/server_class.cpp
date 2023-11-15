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
        ClientHandler *clientHandlerObj = new ClientHandler();
        std::string receivedMessage(buffer, bytesRead);
        std::cout << "Just received message: " << receivedMessage << std::endl;
        std::thread clientThread(&ClientHandler::handleClient, clientHandlerObj, receivedMessage, bytesRead, clientAddr, clientAddrLen, root_dirpath);
        clientThreads.push_back(std::move(clientThread));
    }
}



















void ClientHandler::handleClient(std::string receivedMessage, int bytesRead, sockaddr_in clientAddr, socklen_t clientAddrLen, std::string root_dirpath){
    session.clientAddr = clientAddr;
    session.root_dirpath = root_dirpath;

    if(bytesRead >= 2)
    {
        TFTPPacket *packet = TFTPPacket::parsePacket(receivedMessage);
        if (packet == nullptr)
        {
            //TODO error message
            std::cout << "CHYBA" << std::endl;
            return;
        }
        switch (packet->opcode){
            case Opcode::RRQ:
                session.direction = Direction::Download;
                std::cout << "RRQ packet" << std::endl;
                handlePacket(packet);
                break;
            case Opcode::WRQ:
                session.direction = Direction::Upload;
                std::cout << "WRQ packet" << std::endl;
                handlePacket(packet);
                break;
            default:
                //TODO error packet
                break;
        }

    }
    return;
}


void ClientHandler::handlePacket(TFTPPacket *packet)
{
    if(packet->blksize != -1)   //if not set in rrq/wrq, remain default int session (512)
    {
        session.blksize = packet->blksize;
        session.blksize_set = true;
    }
    session.filename = packet->filename;
    session.timeout = packet->timeout;
    session.tsize = packet->tsize;
    std::cout << "Handling packet" << std::endl;
    if (session.direction == Direction::Upload)
    {
        if (setupFileForUpload() == -1)
        {
            //TODO error packet
            return;
        }
    } else if (session.direction == Direction::Download)
    {
        if (setupFileForDownload() == -1)
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
    if (session.blksize_set == true || session.timeout != -1 || session.tsize != -1)
    {
        OACKPacket OACK_response_packet(session.block_number, session.blksize_set, session.blksize, session.timeout, session.tsize);
        OACK_response_packet.send(session.udpSocket, session.clientAddr);
        std::cout << "Just sent OACK with block number: " << OACK_response_packet.blknum << std::endl;
    } else
    {
        if (session.direction == Direction::Download)
        {
            current_state = TransferState::SendData;
        } else if (session.direction == Direction::Upload)
        {
            current_state = TransferState::SendAck;
        }
    }
    delete packet;
    transferFile();
}

int ClientHandler::createUdpSocket()
{
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        std::cout << "Chyba při vytváření socketu: " << std::endl;
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

int ClientHandler::receiveData()
{
    char received_data[65507]; // Buffer pro přijatou zprávu, max velikost UDP packetu
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    std::cout << "cekam na data" << std::endl;
    int bytesRead = recvfrom(session.udpSocket, received_data, sizeof(received_data), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (bytesRead == StatusCode::CONNECTION_ERROR) {
        std::cout << "Chyba při čekání na zprávu: " << strerror(errno) << std::endl;
        return -1;
    }

    std::string received_message(received_data, bytesRead);
    std::cout << "Prijata zprava:" << received_message << std::endl;
    if (bytesRead < 4)
    {
        //TODO error packet
        return -1;
    }

    TFTPPacket *packet = TFTPPacket::parsePacket(received_message);
    if (packet == nullptr)
    {
        //TODO error packet
        return -1;
    }

    if (packet->opcode != Opcode::DATA)
    {
        //TODO error packet
        return -1;
    }
    if (packet->blknum != session.block_number)
    {
        //TODO osetrit
        std::cout << "spatny block number!" << std::endl;
        return -1;
    }
    //std::vector<char> data_vector(received_data, received_data + bytesRead);
    writeData(packet->data);
    if ((unsigned short int)packet->data.size() < session.blksize)
    {
        session.last_packet = true;
        //closeFile(&(session.file));
    }

    return 0;
}

int ClientHandler::setupFileForDownload()
{
    std::string filename = session.root_dirpath + "/" + session.filename;
    downloaded_file.open(filename);

    if (!(downloaded_file.is_open())) {
        // Soubor se nepodařilo otevřít
        std::cerr << "Failed to open file: " << strerror(errno) << std::endl;
    }

    return 0;
}

int ClientHandler::setupFileForUpload()
{
    std::string filename = session.root_dirpath + "/" + session.filename;
    file.open(filename, std::ios::binary | std::ios::out);
    std::cout << "Filepath: " << filename << std::endl;

    if (!(file).is_open()) {
        std::cerr << "Error opening the file" << std::endl;
        return -1;
    }
    std::cout << "File opened" << std::endl;
    return 0;
}

void ClientHandler::writeData(std::vector<char> data)
{
    file.write(data.data(), data.size());
    return;
}

int ClientHandler::handleSendingData()
{
    std::vector<char> data;
    char buffer[session.blksize];
    downloaded_file.read(buffer, session.blksize);
    data.insert(data.end(), buffer, buffer + session.blksize);
    session.block_number++;
    ssize_t bytesRead = downloaded_file.gcount();

    TFTPPacket::sendData(session.udpSocket, session.clientAddr, session.block_number, session.blksize, bytesRead, data, &(session.last_packet));
    if (session.last_packet == true)
    {
        downloaded_file.close();
    }
}

int ClientHandler::transferFile()
{
    while(session.last_packet == false)
    {
        switch(current_state){
            case TransferState::WaitForTransfer:
                if(session.direction == Direction::Download)
                {
                    current_state = TransferState::SendData;
                } else if (session.direction == Direction::Upload)
                {
                    session.block_number = 1;
                    current_state = TransferState::ReceiveData;
                }
                break;
            case TransferState::SendAck:
                TFTPPacket::sendAck(session.block_number, session.udpSocket, session.clientAddr);
                session.block_number++;
                current_state = TransferState::ReceiveData;
                break;
            case TransferState::ReceiveData:
                //receive data
                TFTPPacket::receiveData(session.udpSocket, session.block_number, session.blksize, &(file), &(session.last_packet));
                current_state = TransferState::SendAck;
                break;
            case TransferState::SendData:
                //send data
                handleSendingData();
                current_state = TransferState::ReceiveAck;
                break;
            case TransferState::ReceiveAck:
                //receive ack
                TFTPPacket::receiveAck(session.udpSocket);
                current_state = TransferState::SendData;
                break;
            case TransferState::SendError:
                //send error
                break;
        }
    }
    TFTPPacket::sendAck(session.block_number, session.udpSocket, session.clientAddr);
    session.block_number++;
    std::cout << "Transfer finished" << std::endl;
    return 0;
}