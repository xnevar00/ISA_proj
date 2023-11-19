// File:    server-class.cpp
// Author:  Veronika Nevarilova
// Date:    11/2023

#include <iostream>
#include "../../include/server/server_class.hpp"
#include <arpa/inet.h>

namespace fs = std::filesystem;

std::atomic<bool> terminateThreads(false);

void signalHandler(int signal) {
    // setting reaction to SIGINT
    OutputHandler::getInstance()->print_to_cout("Signal " + std::to_string(signal) + " received.");
    terminateThreads = true;
}

void setupSignalHandler() {
    std::signal(SIGINT, signalHandler);
}

Server* Server::server_ = nullptr;;

Server *Server::getInstance()
{
    // default settings on server
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
                    return -1;
                } else 
                {
                    server->port = std::atoi(optarg);
                    if (server->port <= 0 || server->port > 65535) 
                    {
                        return -1;
                    }
                    port_set = true;
                }
                break;
            case '?':
                return -1;
            default:
                break;
        }
    }

    if (optind >= argc) {
        return -1;
    }
    if ((argc > 2 && port_set == false) || argc > 4) {
        return -1;
    }

    server->root_dirpath = argv[optind];

    OutputHandler::getInstance()->print_to_cout("Port:" + std::to_string(server->port));
    OutputHandler::getInstance()->print_to_cout("Directory:" + server->root_dirpath);

    return 0;
}

int Server::listen(Server *server)
{
    int udpSocket = initialize_connection(server);
    if (udpSocket == -1) {
        return -1;
    }

    OutputHandler::getInstance()->print_to_cout("Listening...");
    // start to get clients
    server_loop(udpSocket);

    close(udpSocket);

    return 0;
}

int Server::create_udp_socket() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        OutputHandler::getInstance()->print_to_cout("Error creating UDP socket.");
        return -1;
    }
    return udpSocket;
}

int Server::setup_udp_socket(int udpSocket, Server *server){
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server->port); // Port 69 for TFTP
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        OutputHandler::getInstance()->print_to_cout("Error establishing connection.");
        close(udpSocket);
        return -1;
    }
    return udpSocket;
}

int Server::initialize_connection(Server *server)
{
    int udpSocket = create_udp_socket();
    if (udpSocket == -1) {
        return -1;
    }
    udpSocket = setup_udp_socket(udpSocket, server);
    if(udpSocket == -1){
        return -1;
    }
    return udpSocket;

}

void Server::server_loop(int udpSocket) {
    char buffer[MAXMESSAGESIZE];

    //SIGINT handling
    setupSignalHandler();

    std::vector<std::future<void>> futures_vector;

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) 
    {
        OutputHandler::getInstance()->print_to_cout("Error setting socket options: " + std::string(strerror(errno)));
    }

    while (true && !terminateThreads) {
        // getting the clients but periodically checking whether there is SIGINT signal to terminate
        int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesRead == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) 
            {
                // timeout, check if we should terminate
                if (terminateThreads) {
                    OutputHandler::getInstance()->print_to_cout("Terminating the server.");
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
                OutputHandler::getInstance()->print_to_cout("Error waiting for message: " + std::string(strerror(errno)));
                continue;
            }
        }

        // setting new thread for each client
        OutputHandler::getInstance()->print_to_cout("New client!");
        ClientHandler *clientHandlerObj = new ClientHandler();
        std::string receivedMessage(buffer, bytesRead);
        OutputHandler::getInstance()->print_to_cout("Just received message: " + receivedMessage);
        auto future = std::async(std::launch::async, &ClientHandler::handleClient, clientHandlerObj, receivedMessage, bytesRead, clientAddr, root_dirpath);
        futures_vector.push_back(std::move(future));
    }
}














void ClientHandler::handleClient(std::string receivedMessage, int bytesRead, sockaddr_in set_clientAddr, std::string set_root_dirpath){
    clientAddr = set_clientAddr;
    root_dirpath = set_root_dirpath;
    clientPort = getPort(clientAddr);

    if (createUdpSocket() == -1)
    {
        return;
    }

    if(bytesRead >= 2) // checking that at least opcode is there, other checks are in parsePacket
    {
        auto [packet, ok] = TFTPPacket::parsePacket(receivedMessage, getIPAddress(clientAddr), ntohs(clientAddr.sin_port), getLocalPort(udpSocket));
        if (ok == -1)
        {
            TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
            return;
        } else if (ok == -2)
        {
            TFTPPacket::sendError(udpSocket, clientAddr, 8, "Illegal options.");
            return;
        }
        switch (packet->opcode){
            case Opcode::RRQ:
                direction = Direction::Download;
                OutputHandler::getInstance()->print_to_cout("RRQ packet");
                handlePacket(packet);
                break;
            case Opcode::WRQ:
                direction = Direction::Upload;
                OutputHandler::getInstance()->print_to_cout("WRQ packet");
                handlePacket(packet);
                break;
            default:
                TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
                break;
        }
        delete packet;
    } else
    {
        TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
    }
    close(udpSocket);
    return;
}

void ClientHandler::handlePacket(TFTPPacket *packet)
{
    if(packet->blksize != -1)   //if not set in rrq/wrq, remain default (512)
    {
        block_size = packet->blksize;
        block_size_set = true;
    }

    filename = packet->filename;

    if (packet->timeout != -1)  //if not set in rrq/wrq, remain default (5)
    {
        timeout = packet->timeout;
        set_timeout_by_client = packet->timeout;
    }

    tsize = packet->tsize;
    mode = packet->mode;

    // if client sent illegal options, the server will not give them to the oack packet and will use default values
    if (packet->timeout > MAXTIMEOUTVALUE)
    {
        timeout = INITIALTIMEOUT;
        packet->timeout = -1;
    }
    if ((packet->blksize < 8 && block_size_set == true) || (packet->blksize > MAXBLKSIZEVALUE))
    {
        block_size = 512;
        block_size_set = false;
    }
    if ((packet->tsize > MAXTSIZEVALUE && direction == Direction::Upload) || (packet->tsize != 0 && direction == Direction::Download))
    {
        tsize = -1;
        packet->tsize = -1;
    }

    // if tsize was requested/sent byt the client
    if (packet->tsize != -1)
    {
        if (direction == Direction::Upload)
        {
            int ok = isEnoughSpace(root_dirpath, packet->tsize);
            if (ok == 0)
            {
                TFTPPacket::sendError(udpSocket, clientAddr, 3, "Disk full or allocation exceeded.");
                return;
            } else if (ok == -1)
            {
                TFTPPacket::sendError(udpSocket, clientAddr, 0, "Error getting the available disk space.");
                return;
            }
        } else if (direction == Direction::Download)
        {
            if (packet->tsize == 0)
            {
                tsize = getFileSize(root_dirpath, filename);
                if (tsize == -1)
                {
                    TFTPPacket::sendError(udpSocket, clientAddr, 1, "File not found.");
                    return;
                } else if (tsize > MAXTSIZEVALUE)
                {
                    TFTPPacket::sendError(udpSocket, clientAddr, 3, "Disk full or allocation exceeded.");
                    return;
                }
            } else
            {
                OutputHandler::getInstance()->print_to_cout("Illegal tsize value");
                TFTPPacket::sendError(udpSocket, clientAddr, 8, "Illegal option value.");
                return;
            }
        }
    }

    setTimeout(&udpSocket, timeout);
    
    OutputHandler::getInstance()->print_to_cout("Handling packet.");
    // getting ready the files to work with
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

    // if any option was requested, respond with OACK packet
    if (block_size_set == true || packet->timeout != -1 || packet->tsize != -1)
    {
        OACKPacket OACK_response_packet(block_number, block_size_set, block_size, timeout, tsize);
        if (OACK_response_packet.send(udpSocket, clientAddr, &last_data) == -1)
        {
            OutputHandler::getInstance()->print_to_cout("Error sending OACK packet.");
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
    } else  // otherwise send an ACK
    {
        if (direction == Direction::Download)
        {
            current_state = TransferState::SendData;
        } else if (direction == Direction::Upload)
        {
            current_state = TransferState::SendAck;
        }
    }
    if(transferFile() == -1)
    {
        if (direction == Direction::Upload)
        {
            clean(&file, full_filepath);
        } else if (file.is_open())
        {
            file.close();
        }
        return;
    }
}

int ClientHandler::createUdpSocket()
{
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        OutputHandler::getInstance()->print_to_cout("Error creating UDP socket.");
        return -1;
    }

    setTimeout(&udpSocket, timeout);

    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(0);
    serverAddr.sin_addr.s_addr = INADDR_ANY;


    if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        OutputHandler::getInstance()->print_to_cout("Error establishing connection.");
        close(udpSocket);
        return -1;
    }

    // getting info about socket
    struct sockaddr_in localAddress;
    socklen_t addressLength = sizeof(localAddress);
    getsockname(udpSocket, (struct sockaddr*)&localAddress, &addressLength);

    int port = ntohs(localAddress.sin_port);

    OutputHandler::getInstance()->print_to_cout("Server is running on port: " + std::to_string(port));

    return 0;
}

int ClientHandler::setupFileForDownload()
{
    OutputHandler::getInstance()->print_to_cout("Setting up file for download.");
    full_filepath = root_dirpath + "/" + filename;

    if(!(fs::exists(full_filepath)))
    {
        OutputHandler::getInstance()->print_to_cout("File not found.");
        TFTPPacket::sendError(udpSocket, clientAddr, 1, "File not found.");
        return -1;
    }

    downloaded_file.open(full_filepath, std::ios::binary | std::ios::in);
    if (!(downloaded_file).is_open()) {
        TFTPPacket::sendError(udpSocket, clientAddr, 2, "Access violation.");
        return -1;
    }

    return 0;
}

int ClientHandler::setupFileForUpload()
{
    full_filepath = root_dirpath + "/" + filename;
    if(fs::exists(full_filepath))
    {
        OutputHandler::getInstance()->print_to_cout("File already exists.");
        TFTPPacket::sendError(udpSocket, clientAddr, 6, "File already exists.");
        return -1;
    }
         
    file.open(full_filepath, std::ios::binary | std::ios::out);
    OutputHandler::getInstance()->print_to_cout("Filepath" + full_filepath);

    if (!(file).is_open()) {
        TFTPPacket::sendError(udpSocket, clientAddr, 2, "Access violation.");
        return -1;
    }
    OutputHandler::getInstance()->print_to_cout("File opened.");
    return 0;
}

int ClientHandler::writeData(std::vector<char> data)
{
    // write data to the file
    file.write(data.data(), data.size());
    if (file.fail())
    {
        OutputHandler::getInstance()->print_to_cout("Error writing to file maybe because of not enough diskspace.");
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
        // first check if there is any overflow from previous read, add it to the data
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
            // handle the '\r' and '\n' characters
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
    TFTPPacket::sendData(udpSocket, clientAddr, block_number, block_size, bytesRead, data, &(last_packet), &last_data);
    if (last_packet == true)
    {
        downloaded_file.close();
    }
    return 0;
}

int ClientHandler::transferFile()
{
    // finite state machine used for transfer of the data
    int ok;
    while(last_packet == false && !terminateThreads && attempts_to_resend <= MAXRESENDATTEMPTS)
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
                ok = TFTPPacket::sendAck(block_number, udpSocket, clientAddr, &last_data);
                if (ok == -1)
                {
                    OutputHandler::getInstance()->print_to_cout("Error sending ACK packet.");
                    return -1;
                }
                block_number++;
                current_state = TransferState::ReceiveData;
                break;

            case TransferState::ReceiveData:
                ok = TFTPPacket::receiveData(udpSocket, block_number, block_size, &(file), &(last_packet), clientPort, &r_flag, mode);
                if (ok == -1)
                {
                    OutputHandler::getInstance()->print_to_cout("Illegal TFTP operation.");
                    TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP Operation.");
                } else if (ok == -2)
                {
                    // received data from someone else, wait for data from out client
                    OutputHandler::getInstance()->print_to_cout("Unknown transfer ID.");
                    current_state = TransferState::ReceiveData;
                    break;
                } else if (ok == -3)
                {
                    // timeout
                    attempts_to_resend++;
                    if (attempts_to_resend <= MAXRESENDATTEMPTS)
                    {
                        OutputHandler::getInstance()->print_to_cout("Resending ACK... (" + std::to_string(attempts_to_resend) + ")");
                        resendData(udpSocket, clientAddr, last_data);
                    }
                    current_state = TransferState::ReceiveData;
                    // exponential backoff
                    timeout *= 2;
                    setTimeout(&udpSocket, timeout);
                    break;
                }
                // received data, set everything to default
                attempts_to_resend = 0;
                timeout = set_timeout_by_client;
                OutputHandler::getInstance()->print_to_cout("Setting timeout to: " + std::to_string(timeout));
                setTimeout(&udpSocket, timeout);
                current_state = TransferState::SendAck;
                break;

            case TransferState::SendData:
                ok = handleSendingData();
                if (ok == -1)
                {
                    OutputHandler::getInstance()->print_to_cout("Error sending DATA packet.");
                    return -1;
                }
                current_state = TransferState::ReceiveAck;
                break;

            case TransferState::ReceiveAck:
                ok = TFTPPacket::receiveAck(udpSocket, block_number, clientPort);
                if (ok == -1)
                {
                    OutputHandler::getInstance()->print_to_cout("Illegal TFTP operation.");
                    TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
                    return -1;
                } else if (ok == -3)
                {
                    attempts_to_resend++;
                    if (attempts_to_resend <= MAXRESENDATTEMPTS)
                    {
                        OutputHandler::getInstance()->print_to_cout("Resending DATA... (" + std::to_string(attempts_to_resend) + ")");
                        resendData(udpSocket, clientAddr, last_data);
                    }
                    current_state = TransferState::ReceiveAck;
                    // exponential backoff
                    timeout *= 2;
                    OutputHandler::getInstance()->print_to_cout("Setting timeout to: " + std::to_string(timeout));
                    setTimeout(&udpSocket, timeout);
                    break;
                }
                // received ack, set everything to default
                attempts_to_resend = 0;
                timeout = set_timeout_by_client;
                setTimeout(&udpSocket, timeout);
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
                OutputHandler::getInstance()->print_to_cout("Illegal TFTP operation");
                TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
                break;
        }
    }
    // SIGINT was received
    if (terminateThreads)
    {   
        OutputHandler::getInstance()->print_to_cout("Ending thread because of SIGINT.");
        return -1;
    }
    // timed out
    if (attempts_to_resend >= MAXRESENDATTEMPTS)
    {
        OutputHandler::getInstance()->print_to_cout("Client is not responding, aborting.");
        return -1;
    }
    if (direction == Direction::Upload)
    {
        ok = TFTPPacket::sendAck(block_number, udpSocket, clientAddr, &last_data);
        if (ok == -1)
        {
            OutputHandler::getInstance()->print_to_cout("Error sending ACK packet.");
            return -1;
        }
        block_number++;
    } else 
    {        
        ok = -1;
        while(ok != 0 && attempts_to_resend < MAXRESENDATTEMPTS && !terminateThreads)
        {   
            ok = TFTPPacket::receiveAck(udpSocket, block_number, clientPort);
            if (ok == -1)
            {
                OutputHandler::getInstance()->print_to_cout("Illegal TFTP operation.");
                TFTPPacket::sendError(udpSocket, clientAddr, 4, "Illegal TFTP operation.");
                return -1;
            } else if (ok == -3)
            {
                attempts_to_resend++;
                OutputHandler::getInstance()->print_to_cout("Resending DATA... (" + std::to_string(attempts_to_resend) + ")");
                resendData(udpSocket, clientAddr, last_data);
                timeout *= 2;
                OutputHandler::getInstance()->print_to_cout("Setting timeout to: " + std::to_string(timeout));
                setTimeout(&udpSocket, timeout);
            }
        }
        if (terminateThreads)
        {   
            OutputHandler::getInstance()->print_to_cout("Ending thread because of SIGINT.");
            return -1;
        }
    }

    // end of transfer
    OutputHandler::getInstance()->print_to_cout("Transfer finished");
    return 0;
}