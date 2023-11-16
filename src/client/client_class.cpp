#include <iostream>
#include "../../include/client/client_class.hpp"

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
        return StatusCode::INVALID_ARGUMENTS;
    }
    if ((argc < 5) || (argc > 9) || (argc%2 != 1)) {
        return StatusCode::INVALID_ARGUMENTS;
    }
    std::cout << "Hostname: " << hostname << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Filepath: " << filepath << std::endl;
    std::cout << "Dest Filepath: " << destFilepath << std::endl;

    return StatusCode::SUCCESS;
}

int Client::createUdpSocket() {
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        std::cout << "Chyba při vytváření socketu: " << std::endl;
        return -1;
    }
    
    struct sockaddr_in clientAddr;
    std::memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddr.sin_port = htons(0);
    
    if (bind(udpSocket, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) == -1) {
        std::cout << "Chyba při bind: " << std::endl;
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
        std::cout << "Error getting response from server: " << std::endl;
        return -1;
    }

    serverAddr = tmpServerAddr;

    std::string receivedMessage(buffer, bytesRead);

    TFTPPacket *packet = TFTPPacket::parsePacket(receivedMessage, getIPAddress(tmpServerAddr), ntohs(tmpServerAddr.sin_port), getLocalPort(udpSocket));
    if (packet == nullptr)
    {
        //TODO error message
        std::cout << "CHYBA" << std::endl;
        return -1;
    }
    switch(packet->opcode){
        case Opcode::ACK:
            if (direction != Direction::Upload)
            {
                //TODO error packet
                std::cout << "Received ACK packet when expecting DATA or OACK packet." << std::endl;
                return -1;
            }
            std::cout << "Received ACK packet." << std::endl;
            break;
        case Opcode::OACK:
            if (block_size == 513 && timeout == -1 && tsize == -1) //no options sent to server but server responded with OACK packet
            {
                //TODO error packet
                std::cout << "Received OACK packet when expecting DATA or ACK packet." << std::endl;
                return -1;
            }
            std::cout << "Received OACK packet" << std::endl;
            updateAcceptedOptions(packet);
            break;
        case Opcode::DATA:
            if (direction == Direction::Upload)
            {
                //TODO error packet
                std::cout << "Received DATA packet when expecting ACK or OACK packet." << std::endl;
                return -1;
            }
            block_number++;
            file.write(packet->data.data(), packet->data.size());
            break;
        case Opcode::RRQ:
            return -1;
        case Opcode::WRQ:
            return -1;
        case Opcode::ERROR:
            return -1;
        default:
            return -1;
            break;
    }

    transferFile();

    return 0;
}

int Client::handleSendingData()
{
    std::cout << "Beru data ze stdin" << std::endl;
    std::vector<char> data(block_size);
    std::cin.read(data.data(), block_size);
    block_number++;
    ssize_t bytesRead = std::cin.gcount();

    TFTPPacket::sendData(udpSocket, serverAddr, block_number, block_size, bytesRead, data, &last_packet);
    return 0;
}

int Client::transferFile()
{
    while(last_packet == false)
    {
        switch(current_state){
            case TransferState::WaitForTransfer:
                if(direction == Direction::Download)
                {
                    current_state = TransferState::SendAck; //musi se udelat zpracovani oak nebo prvnich dat predtim nez sem vleze
                } else if (direction == Direction::Upload)
                {
                    current_state = TransferState::SendData;
                }
                break;
            case TransferState::SendAck:
                TFTPPacket::sendAck(block_number, udpSocket, serverAddr);
                block_number++;
                current_state = TransferState::ReceiveData;
                break;
            case TransferState::ReceiveData:
                TFTPPacket::receiveData(udpSocket, block_number, block_size, &(file), &(last_packet));
                current_state = TransferState::SendAck;
                break;
            case TransferState::SendData:
                handleSendingData();
                current_state = TransferState::ReceiveAck;
                break;
            case TransferState::ReceiveAck:
                TFTPPacket::receiveAck(udpSocket, block_number);
                current_state = TransferState::SendData;
                break;
            case TransferState::SendError:
                //send error
                break;
        }
    }
    if (direction == Direction::Upload)
    {
        TFTPPacket::receiveAck(udpSocket, block_number);
    } else 
    {
        TFTPPacket::sendAck(block_number, udpSocket, serverAddr);
        block_number++;
    }
    std::cout << "Konec prenosu" << std::endl;
    return 0;
}

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

int Client::setupFileForDownload()
{
    file.open(destFilepath, std::ios::out | std::ios::app | std::ios::binary);
    std::cout << "Filepath: " << destFilepath << std::endl;

    if (!(file).is_open()) {
        std::cerr << "Error opening the file" << std::endl;
        return -1;
    }
    std::cout << "File opened" << std::endl;
    return 0;
}

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
        std::cerr << "Chyba při získávání informací o adrese." << std::endl;
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
        std::cout << "je to upload a Filename: " << filename << std::endl;
        direction = Direction::Upload;
    } else {
        opcode = 1;
        filename = filepath;
        std::cout << "je to download a Filename: " << filename << std::endl;
        direction = Direction::Download;
        if (setupFileForDownload() == -1)
        {
            //TODO error packet
            return -1;
        }
    }

    RRQWRQPacket packet(opcode, filename, "octet", timeout, block_size, tsize); //-1 because of default blocksize
    packet.send(udpSocket, addr);

    return 0;
}

int Client::communicate()
{
    createUdpSocket();
    if (udpSocket == -1) {
        return -1;
    }

    if (sendBroadcastMessage() == -1) {
        close(udpSocket);
        return -1;
    }

    std::cout << "Broadcast message sent." << std::endl;

    if (transferData() == -1) {
        close(udpSocket);
        return -1;
    }

    close(udpSocket);

    return 0;
}