#include <iostream>
#include "../../include/client/client_class.hpp"

Client* Client::client_ = nullptr;;

/**
 * Static methods should be defined outside the class.
 */
Client *Client::getInstance()
{
    /**
     * This is a safer way to create an instance. instance = new Singleton is
     * dangeruous in case two instance threads wants to access at the same time
     */
    if(client_==nullptr){
        client_ = new Client();
        client_->port = 69;
        client_->hostname = "";
        client_->filepath = "";
        client_->destFilepath = "";
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
                session.hostname = optarg;
                break;
            case 'p':
                if (str_is_digits_only(optarg) == false) 
                {
                    return -1;
                } else 
                {
                    session.initial_port = std::atoi(optarg);
                    if (session.initial_port <= 0 || session.initial_port > 65535) 
                    {
                        return -1;
                    }
                }
                break;
            case 'f':
                std::cout << "filepath:" << optarg << std::endl;
                session.filepath = optarg;
                break;
            case 't':
                std::cout << "dest_filepath:" << optarg << std::endl;
                session.destFilepath = optarg;
                break;
            case '?':
                return -1;
            default:
                break;
        }
    }

    if (session.hostname.empty() || session.destFilepath.empty()) {
        return StatusCode::INVALID_ARGUMENTS;
    }
    if ((argc < 5) || (argc > 9) || (argc%2 != 1)) {
        return StatusCode::INVALID_ARGUMENTS;
    }
    std::cout << "Hostname: " << session.hostname << std::endl;
    std::cout << "Port: " << session.initial_port << std::endl;
    std::cout << "Filepath: " << session.filepath << std::endl;
    std::cout << "Dest Filepath: " << session.destFilepath << std::endl;

    return StatusCode::SUCCESS;
}

int Client::create_udp_socket() {
    //creating socket
    session.udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (session.udpSocket == -1) {
        std::cout << "Chyba při vytváření socketu: " << std::endl;
        return -1;
    }
    // end of creating socket

    
    struct sockaddr_in clientAddr;
    std::memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddr.sin_port = htons(0);
    

    if (bind(session.udpSocket, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) == -1) {
        std::cout << "Chyba při bind: " << std::endl;
        close(session.udpSocket);
        return -1;
    }

    // getting the number of client's port
    struct sockaddr_in localAddress;
    socklen_t addressLength = sizeof(localAddress);
    getsockname(session.udpSocket, (struct sockaddr*)&localAddress, &addressLength);


    int port = ntohs(localAddress.sin_port);

    std::cout << "Klientský socket běží na portu: " << port << std::endl;
    // end of getting the number of client's port

    return 0;
}


int Client::receive_respond_from_server() {
    char buffer[1024]; // Buffer pro přijatou odpověď
    struct sockaddr_in serverAddr;
    socklen_t serverAddrLen = sizeof(serverAddr);

    int bytesRead = recvfrom(session.udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&serverAddr, &serverAddrLen);
    if (bytesRead == -1) {
        std::cout << "Chyba při přijímání odpovědi: " << strerror(errno) << std::endl;
        return -1;
    }

    session.serverTID = ntohs(session.serverAddr.sin_port);
    session.serverAddr = serverAddr;

    std::string receivedMessage(buffer, bytesRead);
    std::cout << "Přijata odpověď od serveru: " << receivedMessage << std::endl;


    TFTPPacket *packet = TFTPPacket::parsePacket(receivedMessage);
    if (packet == nullptr)
    {
        //TODO error message
        std::cout << "CHYBA" << std::endl;
        return -1;
    }
    switch(packet->opcode){
        case Opcode::ACK:
            std::cout << "dostal jsem ACK" << std::endl;
            break;
        case Opcode::OACK:
            std::cout << "dostal jsem OACK" << std::endl;
            updateAcceptedOptions(packet);
            break;
        default:
            return -1;
            break;
    }
    transferFile();

    //test odeslani odpovedi serveru z portu, ze ktereho mi odpovedel
    /*if (sendto(session.udpSocket, "ahoj\0",5, 0, (struct sockaddr*)&session.serverAddr, sizeof(session.serverAddr)) == -1) {
        std::cout << "Chyba při odesílání broadcast zprávy: " << strerror(errno) << std::endl;
        close(session.udpSocket);
        return -1;
    }
    std::cout << "odeslana zprava Ahoj" << std::endl; */


    return 0;
}

int Client::sendAck()
{
    block_number++;
    ACKPacket response_packet(block_number);
    if (response_packet.send(session.udpSocket, session.serverAddr) == -1)
    {
        return -1;
    }
    std::cout << "Just sent ack packet" << std::endl;
    return 0;
}

int Client::receiveData()
{
    char received_data[65507]; // Buffer pro přijatou zprávu, max velikost UDP packetu
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);

    std::cout << "cekam na data" << std::endl;
    int bytesRead = recvfrom(session.udpSocket, received_data, sizeof(received_data), 0, (struct sockaddr*)&addr, &addrLen);
    if (bytesRead == -1) {
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
    if (packet->blknum != block_number)
    {
        //TODO osetrit
        std::cout << "spatny block number!" << std::endl;
        return -1;
    }
    std::vector<char> data_vector(received_data, received_data + bytesRead);
    writeData(packet->data);
    std::cout << "obdrzeno bytu: " << bytesRead << std::endl;
    if (packet->data.size() < block_size)
    {
        last_packet = true;
        closeFile(&(file));
    }

    return 0;
}

void Client::writeData(std::vector<char> data)
{
    file.write(data.data(), data.size());
    return;
}

int Client::sendData()
{
    std::cout << "Beru data ze stdin" << std::endl;
    std::vector<char> data(block_size);
    std::cin.read(data.data(), block_size);
    block_number++;
    ssize_t bytesRead = std::cin.gcount();
    
    if (bytesRead <= 0) {
        std::cout << "Chyba při čtení ze stdin" << std::endl;
        return -1;
    }

    data.resize(bytesRead);
    if (data.size() < block_size)
    {
        std::cout << "posledni packet" << std::endl;
        last_packet = true;
    }
    DATAPacket response_packet(block_number, data);
    if (response_packet.send(session.udpSocket, session.serverAddr) == -1)
    {
        return -1;
    }
    std::cout << "Odeslan data packet" << std::endl;
    return 0;
}

int Client::receiveAck()
{
    std::cout << "cekam na ack" << std::endl;
    char received_data[65507]; // Buffer pro přijatou zprávu, max velikost UDP packetu
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);

    int bytesRead = recvfrom(session.udpSocket, received_data, sizeof(received_data), 0, (struct sockaddr*)&addr, &addrLen);
    if (bytesRead == -1) {
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
    if (packet->opcode != Opcode::ACK)
    {
        //TODO error packet
        return -1;
    }
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
                sendAck();
                current_state = TransferState::ReceiveData;
                break;
            case TransferState::ReceiveData:
                //receive data
                receiveData();
                current_state = TransferState::SendAck;
                break;
            case TransferState::SendData:
                //send data
                sendData();
                current_state = TransferState::ReceiveAck;
                break;
            case TransferState::ReceiveAck:
                receiveAck();
                current_state = TransferState::SendData;
                break;
            case TransferState::SendError:
                //send error
                break;
        }
    }
    receiveAck();
    std::cout << "Konec prenosu" << std::endl;
    //sendAck();
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
    file.open(session.destFilepath, std::ios::out | std::ios::app | std::ios::binary);
    std::cout << "Filepath: " << session.destFilepath << std::endl;

    if (!(file).is_open()) {
        std::cerr << "Error opening the file" << std::endl;
        return -1;
    }
    std::cout << "File opened" << std::endl;
    return 0;
}

int Client::send_broadcast_message() {
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(session.initial_port); // Port 69 pro TFTP

    // Převod hostname na IP adresu
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    if (getaddrinfo(session.hostname.c_str(), nullptr, &hints, &res) != 0) {
        std::cerr << "Chyba při získávání informací o adrese." << std::endl;
        return -1;
    }

    struct sockaddr_in *addr_in = (struct sockaddr_in *)res->ai_addr;
    addr.sin_addr = addr_in->sin_addr;

    freeaddrinfo(res); // Uvolnění struktury addrinfo
    // konec prevodu hostname na IP adresu

    int opcode;
    std::string filename;
    if (session.filepath == "")
    {
        opcode = 2;
        filename = session.destFilepath;
        std::cout << "je to upload a Filename: " << filename << std::endl;
        direction = Direction::Upload;
    } else {
        opcode = 1;
        filename = session.filepath;
        std::cout << "je to download a Filename: " << filename << std::endl;
        direction = Direction::Download;
        if (this->setupFileForDownload() == -1)
        {
            //TODO error packet
            return -1;
        }
    }

    RRQWRQPacket packet(opcode, filename, "octet", -1, 512, -1);
    packet.send(session.udpSocket, addr);

    return 0;
}

int Client::communicate()
{
    create_udp_socket();
    if (session.udpSocket == -1) {
        return -1;
    }

    if (send_broadcast_message() == -1) {
        close(session.udpSocket);
        return -1;
    }

    std::cout << "Broadcast message sent." << std::endl;

    if (receive_respond_from_server() == -1) {
        close(session.udpSocket);
        return -1;
    }

    close(session.udpSocket);

    return 0;
}