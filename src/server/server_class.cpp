#include <iostream>
#include "../../include/server/server_class.hpp"
#include <arpa/inet.h>  // pro inet_ntoa

namespace fs = std::filesystem;

std::atomic<bool> terminateThreads(false);

void signalHandler(int signum) {
    terminateThreads = true;
}

void setupSignalHandler() {
    std::signal(SIGINT, signalHandler);
}


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

    //SIGINT handling
    setupSignalHandler();

    std::vector<std::future<void>> futures_vector;

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    std::cerr << "Error setting socket options: " << strerror(errno) << std::endl;
    }

    while (true && !terminateThreads) {
        int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesRead == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                // Timeout, check if we should terminate
                if (terminateThreads) {
                    std::cout << "Terminating the server." << std::endl;
                    for (auto& future : futures_vector) {
                        future.get();
                    }
                    break;
                }
                futures_vector.erase(std::remove_if(futures_vector.begin(), futures_vector.end(), [](std::future<void>& ftr) {
                    return ftr.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
                }), futures_vector.end());
                continue;
            } else {
                std::cout << "Error waiting for message: " << strerror(errno) << std::endl;
                continue;
            }
        }

        std::cout << "New client!" << std::endl;
        ClientHandler *clientHandlerObj = new ClientHandler();
        std::string receivedMessage(buffer, bytesRead);
        std::cout << "Just received message: " << receivedMessage << std::endl;
        auto future = std::async(std::launch::async, &ClientHandler::handleClient, clientHandlerObj, receivedMessage, bytesRead, clientAddr, root_dirpath);
        futures_vector.push_back(std::move(future));
    }
    close(udpSocket);
}














//*********************ERRORS HANDLED*****************/
void ClientHandler::handleClient(std::string receivedMessage, int bytesRead, sockaddr_in set_clientAddr, std::string set_root_dirpath){
    clientAddr = set_clientAddr;
    root_dirpath = set_root_dirpath;
    clientPort = getPort(clientAddr);

    if(bytesRead >= 2)
    {
        TFTPPacket *packet = TFTPPacket::parsePacket(receivedMessage, getIPAddress(clientAddr), ntohs(clientAddr.sin_port), getLocalPort(udpSocket));
        if (packet == nullptr)
        {
            TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
            return;
        }
        if (packet->blksize < 8)
        {
            TFTPPacket::sendError(udpSocket, clientAddr, 8, "Illegal options.");
            return;
        }
        switch (packet->opcode){
            case Opcode::RRQ:
                direction = Direction::Download;
                std::cout << "RRQ packet" << std::endl;
                handlePacket(packet);
                break;
            case Opcode::WRQ:
                direction = Direction::Upload;
                std::cout << "WRQ packet" << std::endl;
                handlePacket(packet);
                break;
            default:
                TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
                break;
        }
        delete packet;
    }
    return;
}

//*********************ERRORS HANDLED*****************/
void ClientHandler::handlePacket(TFTPPacket *packet)
{
    if (createUdpSocket() == -1)
    {
        return;
    }

    if(packet->blksize != -1)   //if not set in rrq/wrq, remain default (512)
    {
        block_size = packet->blksize;
        block_size_set = true;
    }
    filename = packet->filename;
    timeout = packet->timeout;
    tsize = packet->tsize;
    mode = packet->mode;
    
    std::cout << "Handling packet" << std::endl;
    if (direction == Direction::Upload)
    {
        if (setupFileForUpload() == -1)
        {
            //error already handled
            return;
        }
    } else if (direction == Direction::Download)
    {
        if (setupFileForDownload() == -1)
        {
            //error already handled
            return;
        }
    }
    if (block_size_set == true || timeout != -1 || tsize != -1)
    {
        OACKPacket OACK_response_packet(block_number, block_size_set, block_size, timeout, tsize);
        if (OACK_response_packet.send(udpSocket, clientAddr) == -1)
        {
            std::cout << "Error sending OACK packet." << std::endl;
            return;
        }

        if (direction == Direction::Download)
        {
            current_state = TransferState::ReceiveAck;
        } else if (direction == Direction::Upload)
        {
            block_number++;
            current_state = TransferState::ReceiveData;
        }
    } else
    {
        if (direction == Direction::Download)
        {
            current_state = TransferState::SendData;
        } else if (direction == Direction::Upload)
        {
            current_state = TransferState::SendAck;
        }
    }
    transferFile();
    close(udpSocket);
}

//**************ERRORS HANDLED*****************/
int ClientHandler::createUdpSocket()
{
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        std::cout << "Error creating UDP socket." << std::endl;
        return -1;
    }

    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(0);
    serverAddr.sin_addr.s_addr = INADDR_ANY;


    if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cout << "Error establishing connection." << strerror(errno) << std::endl;
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
    struct sockaddr_in tmpClientAddr;
    socklen_t tmpClientAddrLen = sizeof(tmpClientAddr);

    std::cout << "cekam na data" << std::endl;
    int bytesRead = recvfrom(udpSocket, received_data, sizeof(received_data), 0, (struct sockaddr*)&tmpClientAddr, &tmpClientAddrLen);
    if (bytesRead == StatusCode::CONNECTION_ERROR) {
        std::cout << "Chyba při čekání na zprávu: " << strerror(errno) << std::endl;
        return -1;
    }
    if (getPort(tmpClientAddr) != clientPort)
    {
        TFTPPacket::sendError(udpSocket, tmpClientAddr, 5, "Unknown transfer ID.");
        return -1;
    }

    std::string received_message(received_data, bytesRead);
    std::cout << "Prijata zprava:" << received_message << std::endl;
    if (bytesRead < 4)
    {
        TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
        return -1;
    }

    TFTPPacket *packet = TFTPPacket::parsePacket(received_message, getIPAddress(clientAddr), ntohs(clientAddr.sin_port), getLocalPort(udpSocket));
    if (packet == nullptr)
    {
        TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
        return -1;
    }

    if (packet->opcode != Opcode::DATA)
    {
        TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
        return -1;
    }
    if (packet->blknum != block_number)
    {
        
        TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
        return -1;
    }
    if (writeData(packet->data) == -1)
    {
        //error already handled
        return -1;
    }
    if ((unsigned short int)packet->data.size() < block_size)
    {
        last_packet = true;
    }

    return 0;
}

int ClientHandler::setupFileForDownload()
{
    std::cout << filename << std::endl;
    std::string filename_path = root_dirpath + "/" + filename;

    if(!(fs::exists(filename_path)))
    {
        std::cout << "File not found." << std::endl;
        TFTPPacket::sendError(udpSocket, clientAddr, 6, "File not found.");
        return -1;
    }

    downloaded_file.open(filename_path, std::ios::binary | std::ios::in);
    if (!(downloaded_file).is_open()) {
        TFTPPacket::sendError(udpSocket, clientAddr, 2, "Access violation.");
        return -1;
    }

    return 0;
}

int ClientHandler::setupFileForUpload()
{
    std::string filename_path = root_dirpath + "/" + filename;
    if(fs::exists(filename_path))
    {
        std::cout << "File already exists." << std::endl;
        TFTPPacket::sendError(udpSocket, clientAddr, 6, "File already exists.");
        return -1;
    }
         
    file.open(filename_path, std::ios::binary | std::ios::out);
    std::cout << "Filepath: " << filename_path << std::endl;

    if (!(file).is_open()) {
        TFTPPacket::sendError(udpSocket, clientAddr, 2, "Access violation.");
        return -1;
    }
    std::cout << "File opened." << std::endl;
    return 0;
}

int ClientHandler::writeData(std::vector<char> data)
{
    file.write(data.data(), data.size());
    if (file.fail())
    {
        std::cerr << "Error writing to file maybe because of not enough diskspace." << std::endl;
        TFTPPacket::sendError(udpSocket, clientAddr, 3, "Disk full or allocation exceeded.");
        return -1;
    }
    return 0;
}

int ClientHandler::handleSendingData()
{
    std::vector<char> data;
    ssize_t bytesRead = 0;
    char c;
    char buffer[block_size];
    if (mode == "octet")
    {
        downloaded_file.read(buffer, block_size);
        if (downloaded_file.fail() && !downloaded_file.eof())
        {
            TFTPPacket::sendError(udpSocket, clientAddr, 3, "Disk full or allocation exceeded.");
            return -1;
        }
        data.insert(data.end(), buffer, buffer + block_size);
        bytesRead = downloaded_file.gcount();
    } else if (mode == "netascii")
    {
        downloaded_file.get(c);
        if (overflow != "")
        {
            while(overflow != "" && bytesRead < block_size)
            {
                data.push_back(overflow[0]);
                overflow.erase(0, 1);
                bytesRead++;
            }
        }
        while(downloaded_file.good() && bytesRead < block_size)
        {
            if (c == '\n')
            {
                data.push_back('\r');
                bytesRead++;
                if ((int)data.size() < block_size)
                {
                    data.push_back('\n');
                    bytesRead ++;
                }  else
                {
                    overflow += "\n";
                }
            } else if (c == '\r')
            {
                data.push_back('\r');
                bytesRead++;
                if ((int)data.size() < block_size)
                {
                    data.push_back('\0');
                    bytesRead++;
                } else
                {
                    overflow += "\0";
                }
            } else
            {
                data.push_back(c);
                bytesRead++;
            }
            downloaded_file.get(c);
        }
        overflow += c;
        if (downloaded_file.fail() && !downloaded_file.eof())
        {
            TFTPPacket::sendError(udpSocket, clientAddr, 3, "Disk full or allocation exceeded.");
            return -1;
        }
    }

    block_number++;
    TFTPPacket::sendData(udpSocket, clientAddr, block_number, block_size, bytesRead, data, &(last_packet));
    if (last_packet == true)
    {
        downloaded_file.close();
    }
    return 0;
}

int ClientHandler::transferFile()
{
    int ok;
    while(last_packet == false && !terminateThreads)
    {
        switch(current_state){
            case TransferState::WaitForTransfer:
                if(direction == Direction::Download)
                {
                    current_state = TransferState::SendData;
                } else if (direction == Direction::Upload)
                {
                    block_number = 1;
                    current_state = TransferState::ReceiveData;
                }
                break;
            case TransferState::SendAck:
                ok = TFTPPacket::sendAck(block_number, udpSocket, clientAddr);
                if (ok == -1)
                {
                    std::cout << "Error sending ACK packet." << std::endl;
                    return -1;
                }
                block_number++;
                current_state = TransferState::ReceiveData;
                break;
            case TransferState::ReceiveData:
                //receive data
                ok = TFTPPacket::receiveData(udpSocket, block_number, block_size, &(file), &(last_packet), clientPort, &r_flag, mode);
                if (ok == -1)
                {
                    std::cout << "Illegal TFTP operation." << std::endl;
                    TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP Operation.");
                } else if (ok == -2)
                {
                    std::cout << "Unknown transfer ID." << std::endl;
                    current_state = TransferState::ReceiveData;
                    break;
                }
                current_state = TransferState::SendAck;
                break;
            case TransferState::SendData:
                //send data
                ok = handleSendingData();
                if (ok == -1)
                {
                    std::cout << "Failed to send DATA packet" << std::endl;
                    return -1;
                }
                current_state = TransferState::ReceiveAck;
                break;
            case TransferState::ReceiveAck:
                //receive ack
                ok = TFTPPacket::receiveAck(udpSocket, block_number, clientPort);
                if (ok == -1)
                {
                    std::cout << "Illegal TFTP operation" << std::endl;
                    TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
                    return -1;
                } 
                if (last_packet == true)
                {
                    current_state = TransferState::WaitForTransfer;
                }
                else
                {
                    current_state = TransferState::SendData;
                }
                break;
            default:
                std::cout << "Illegal TFTP operation" << std::endl;
                TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
                break;
        }
    }
    if (terminateThreads)
    {   
        std::cout << "Ending thread because of SIGINT." << std::endl;
        return -1;
    }
    if (direction == Direction::Upload)
    {
        ok = TFTPPacket::sendAck(block_number, udpSocket, clientAddr);
        if (ok == -1)
        {
            std::cout << "Error sending ACK packet." << std::endl;
            return -1;
        }
        block_number++;
    } else 
    {        
        ok = TFTPPacket::receiveAck(udpSocket, block_number, clientPort);
        if (ok == -1)
        {
            std::cout << "Illegal TFTP operation" << std::endl;
            TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
            return -1;
        } 
    }
    std::cout << "Transfer finished" << std::endl;
    return 0;
}