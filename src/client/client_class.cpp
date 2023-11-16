#include <iostream>
#include "../../include/client/client_class.hpp"

namespace fs = std::filesystem;

Client* Client::client_ = nullptr;;

Client *Client::getInstance()
{
    if(client_==nullptr){
        client_ = new Client();
        client_->port = 69;
        client_->hostname = "";
        client_->filepath = "";
        client_->destFilepath = "";
        client_->udpSocket = -1;
        client_->last_packet = false;
        client_->block_number = 0;
        client_->block_size = 512;
        client_->timeout = -1;
        client_->tsize = -1;
        client_->current_state = TransferState::WaitForTransfer;
    }
    return client_;
}


//*****************ERRORS HANDLED********************/
int Client::parse_arguments(int argc, char* argv[])
{
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
                hostname = optarg;
                break;
            case 'p':
                if (str_is_digits_only(optarg) == false) 
                {
                    return -1;
                } else 
                {
                    port = std::atoi(optarg);
                    if (port < MINPORTVALUE || port > MAXPORTVALUE) 
                    {
                        return -1;
                    }
                }
                break;
            case 'f':
                filepath = optarg;
                break;
            case 't':
                destFilepath = optarg;
                break;
            case '?':
                return -1;
            default:
                break;
        }
    }

    if (hostname.empty() || destFilepath.empty()) {
        return -1;
    }
    if ((argc < 5) || (argc > 9) || (argc%2 != 1)) {
        return -1;
    }
    std::cout << "Hostname: " << hostname << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Filepath: " << filepath << std::endl;
    std::cout << "Dest Filepath: " << destFilepath << std::endl;

    return 0;
}


//**************ERRORS HANDLED**********************/
int Client::createUdpSocket() {
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        std::cout << "Chyba při vytváření socketu." << std::endl;
        return -1;
    }
    
    struct sockaddr_in clientAddr;
    std::memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddr.sin_port = htons(0);
    
    if (bind(udpSocket, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) == -1) {
        std::cout << "Chyba při bind." << std::endl;
        close(udpSocket);
        return -1;
    }

    struct sockaddr_in localAddress;
    socklen_t addressLength = sizeof(localAddress);
    getsockname(udpSocket, (struct sockaddr*)&localAddress, &addressLength);

    //for stderr prints
    int clientPort = ntohs(localAddress.sin_port);
    std::string clientIP = getIPAddress(localAddress);

    return 0;
}

int Client::transferData() {
    char buffer[65507]; // Buffer pro přijatou odpověď
    struct sockaddr_in tmpServerAddr;
    socklen_t tmpServerAddrLen = sizeof(tmpServerAddr);

    int bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&tmpServerAddr, &tmpServerAddrLen);
    if (bytesRead == -1) {
        std::cout << "Error getting response from server." << std::endl;
        return -1;
    }

    serverAddr = tmpServerAddr;

    std::string receivedMessage(buffer, bytesRead);

    TFTPPacket *packet = TFTPPacket::parsePacket(receivedMessage, getIPAddress(tmpServerAddr), ntohs(tmpServerAddr.sin_port), getLocalPort(udpSocket));
    if (packet == nullptr)
    {
        std::cout << "Illegal TFTP operation." << std::endl;
        TFTPPacket::sendError(udpSocket, serverAddr, 4, "Illegal TFTP operation.");
        return -1;
    }

    switch(packet->opcode){
        case Opcode::ACK:
            if (direction != Direction::Upload)
            {
                //TODO error packet
                std::cout << "Received ACK packet when expecting DATA or OACK packet." << std::endl;
                TFTPPacket::sendError(udpSocket, serverAddr, 4, "Illegal TFTP operation.");
                return -1;
            }
            std::cout << "Received ACK packet." << std::endl;
            break;

        case Opcode::OACK:
            if (block_size == 513 && timeout == -1 && tsize == -1) //no options sent to server but server responded with OACK packet
            {
                //TODO error packet
                std::cout << "Received OACK packet when expecting DATA or ACK packet." << std::endl;
                TFTPPacket::sendError(udpSocket, serverAddr, 4, "Illegal TFTP operation.");
                return -1;
            }
            std::cout << "Received OACK packet" << std::endl;
            updateAcceptedOptions(packet);
            break;

        case Opcode::DATA:
            if (direction == Direction::Upload)
            {
                std::cout << "Received DATA packet when expecting ACK or OACK packet." << std::endl;
                TFTPPacket::sendError(udpSocket, serverAddr, 4, "Illegal TFTP operation.");
                return -1;
            }
            block_number++;
            file.write(packet->data.data(), packet->data.size());
            break;

        case Opcode::RRQ:
            TFTPPacket::sendError(udpSocket, serverAddr, 4, "Illegal TFTP operation.");
            return -1;

        case Opcode::WRQ:
            TFTPPacket::sendError(udpSocket, serverAddr, 4, "Illegal TFTP operation.");
            return -1;

        case Opcode::ERROR:
            return -1;
            //handle error
        default:
            return -1;
            break;
    }

    if (transferFile() == -1)
    {
        //error handled in transferFile
        return -1;
    }

    return 0;
}

//*******************ERRORS HANDLED**********************/
int Client::handleSendingData()
{
    std::cout << "Beru data ze stdin" << std::endl;
    std::vector<char> data(block_size);
    std::cin.read(data.data(), block_size);
    block_number++;
    ssize_t bytesRead = std::cin.gcount();

    int ok = TFTPPacket::sendData(udpSocket, serverAddr, block_number, block_size, bytesRead, data, &last_packet);
    if (ok == -1)
    {
        return -1;     //error already handled in sendData
    }
    return 0;
}

//*****************ERRORS HANDLED**********************/
int Client::transferFile()
{
    int ok = 0;
    while(last_packet == false)
    {
        switch(current_state){
            case TransferState::WaitForTransfer:
                if(direction == Direction::Download)
                {
                    current_state = TransferState::SendAck;
                } else if (direction == Direction::Upload)
                {
                    current_state = TransferState::SendData;
                }
                break;

            case TransferState::SendAck:
                ok = TFTPPacket::sendAck(block_number, udpSocket, serverAddr);
                if (ok == -1)
                {
                    std::cout << "Failed to send ACK packet" << std::endl;
                    return -1;
                }
                block_number++;
                current_state = TransferState::ReceiveData;
                break;
                
            case TransferState::ReceiveData:
                ok = TFTPPacket::receiveData(udpSocket, block_number, block_size, &(file), &(last_packet));
                if (ok == -1)
                {
                    std::cout << "Illegal TFTP operation" << std::endl;
                    TFTPPacket::sendError(udpSocket, serverAddr, 4, "Illegal TFTP operation.");
                    return -1;
                }
                current_state = TransferState::SendAck;
                break;

            case TransferState::SendData:
                ok = handleSendingData();
                if (ok == -1)
                {
                    std::cout << "Failed to send DATA packet" << std::endl;
                    return -1;
                }
                current_state = TransferState::ReceiveAck;
                break;

            case TransferState::ReceiveAck:
                ok = TFTPPacket::receiveAck(udpSocket, block_number);
                if (ok == -1)
                {
                    std::cout << "Illegal TFTP operation" << std::endl;
                    TFTPPacket::sendError(udpSocket, serverAddr, 4, "Illegal TFTP operation.");
                    return -1;
                }
                current_state = TransferState::SendData;
                break;
            case TransferState::SendError:
                //send error
                break;
        }
    }
    if (direction == Direction::Upload)
    {
        ok = TFTPPacket::receiveAck(udpSocket, block_number);
        if (ok == -1)
        {
            std::cout << "Illegal TFTP operation" << std::endl;
            TFTPPacket::sendError(udpSocket, serverAddr, 4, "Illegal TFTP operation.");
            return -1;
        }
    } else 
    {
        ok = TFTPPacket::sendAck(block_number, udpSocket, serverAddr);
        if (ok == -1)
        {
            std::cout << "Failed to send ACK packet" << std::endl;
            return -1;
        }
        block_number++;
    }
    std::cout << "End of transmission." << std::endl;
    return 0;
}

//***********ERRORS HANDLED****************/
void Client::updateAcceptedOptions(TFTPPacket *packet)
{
    if (packet->blksize == -1 && block_size != 512)
    {
        block_size = 512;
    }
    if (packet->timeout == -1 && timeout != -1)
    {
        timeout = -1;
    }
    if (packet->tsize == -1 && tsize != -1)
    {
        tsize = -1;
    }
    return;
}


//*********ERRORS HANDLED*******************/
int Client::setupFileForDownload()
{
    std::cout << "Filepath: " << destFilepath << std::endl;

    file.open(destFilepath, std::ios::out | std::ios::binary);

    if (!(file).is_open()) {
        std::cout << "Error opening the file." << std::endl;
        return -1;
    }
    std::cout << "File opened." << std::endl;
    return 0;
}

//****************ERRORS HANDLED********************/
int Client::sendBroadcastMessage() {
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port); // Port 69 pro TFTP

    // Převod hostname na IP adresu
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    if (getaddrinfo(hostname.c_str(), nullptr, &hints, &res) != 0) {
        std::cout << "Chyba při získávání informací o adrese." << std::endl;
        return -1;
    }

    struct sockaddr_in *addr_in = (struct sockaddr_in *)res->ai_addr;
    addr.sin_addr = addr_in->sin_addr;

    freeaddrinfo(res); // Uvolnění struktury addrinfo
    // konec prevodu hostname na IP adresu

    int opcode;
    std::string filename;
    if (filepath == "")
    {
        opcode = 2;
        filename = destFilepath;
        std::cout << "Upload. Filename: " << filename << std::endl;
        direction = Direction::Upload;
    } else {
        opcode = 1;
        filename = filepath;
        std::cout << "Download. Filename: " << filename << std::endl;
        direction = Direction::Download;
        if (setupFileForDownload() == -1)
        {
            // error already handled
            return -1;
        }
    }

    RRQWRQPacket packet(opcode, filename, "octet", timeout, block_size, tsize); //-1 because of default blocksize
    if(packet.send(udpSocket, addr) == -1)
    {
        std::cout << "Error sending packet." << std::endl;
        return -1;
    }

    return 0;
}

//***********ERRORS HANDLED*******************/
int Client::communicate()
{
    createUdpSocket();
    if (udpSocket == -1) {
        //error already handled
        return -1;
    }

    if (sendBroadcastMessage() == -1) {
        //error already handled
        close(udpSocket);
        return -1;
    }

    std::cout << "Broadcast message sent." << std::endl;

    if (transferData() == -1) {
        //error already handled
        close(udpSocket);
        return -1;
    }

    close(udpSocket);

    return 0;
}