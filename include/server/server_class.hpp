/**
 * @file server_class.hpp
 * @author Veronika Nevarilova
 * @date 11/2023
 */

#include <iostream>
#include <getopt.h>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <string>
#include <atomic>
#include <signal.h>
#include <csignal>
#include <future>
#include <algorithm>
#include "../../include/packet/tftp-packet-class.hpp"

/**
 * @brief Function used to handle the SIGINT signal.
 * @param signal received signal.
 */
void signalHandler(int signal);


/**
 * @brief Sets up the signal handler for SIGINT.
 */
void setupSignalHandler();


/**
 * @class Server
 * @brief A class that represents a TFTP server as a singleton.
 *
 * This class provides methods to send and receive TFTP packets to and from a TFTP client.
 */
class Server
{

protected:
    Server(){}

    static Server* server_;
    std::vector<std::thread> clientThreads;        // vector of threads that handle the clients

    /**
     * @brief Creates a UDP socket.
     * @return Created socket.
     */
    int create_udp_socket();

    /**
     * @brief Sets up a UDP socket.
     * @param udpSocket The UDP socket to set up.
     * @param server The server instance.
     * @return 0 if successful.
     */
    int setup_udp_socket(int udpSocket, Server *server);

    /**
     * @brief Initializes the connection with the client.
     * @param server The server instance.
     * @return 0 if successful.
     */
    int initialize_connection(Server *server);

    /**
     * @brief Loop that is getting new clients on default port
     * @param udpSocket UDP socket used for getting new clients
     * @return 0 if successful.
     */
    void server_loop(int udpSocket);

public:
    Server(Server &other) = delete;
    
    int port;                       // port to listen on set by the user, default 69
    std::string root_dirpath;       // path to the root directory set by the user


    void operator=(const Server &) = delete;

    /**
     * @brief Get the single instance of the Server class.
     * @return A pointer to the single instance of the Server class.
     */
    static Server *getInstance();

    /**
     * @brief Parse the command line arguments.
     * @param argc The number of command line arguments.
     * @param argv The command line arguments.
     * @param server The server instance.
     * @return 0 if successful.
     */
    int parse_arguments(int argc, char* argv[], Server *server);

    /**
     * @brief Set up the server and start getting new clients.
     * @param server The server instance.
     * @return 0 if successful.
     */
    int listen(Server *server);
};

/**
 * @class ClientHandler
 * @brief A class that handles client connections for the TFTP server.
 *
 */
class ClientHandler {
    public:
        std::ofstream file;                 // file to be worked with (upload)
        std::ifstream downloaded_file;      // file to be worked with (download)
        std::string filename;               // name of the file to be worked with
        std::string mode;                   // set mode (octet or netascii) by the client          
        TransferState current_state = TransferState::WaitForTransfer;   // current state of the transfer
        int clientPort;                     // client port (used for printing to stderr)
        std::string clientIP;               // client IP (used for printing to stderr)
        sockaddr_in clientAddr;             // client address
        int direction;                      // download = rrq, upload = wrq
        unsigned short block_number;        // number of current block transfered
        int64_t tsize;                      // option tsize value (-1 if not set)
        int64_t timeout;                    // timeout value (default 2)
        int64_t block_size;                 // block size value (default 512)
        bool block_size_set;                // true if block size option was set
        bool last_packet;                   // true if last packet was received/sent, false otherwise
        std::string root_dirpath;           // path to the root directory
        int udpSocket;                      // udp socket used to communicate with the client
        bool r_flag;                        // flag if in the last packet on the last index was '\r' character (used for netascii mode)
        std::string overflow;               // overflow from the last packet (used for netascii mode)
        std::vector<char> last_data;        // last data sent - in case of timeout this will be resent
        int attempts_to_resend;             // number of attempts to resend the last packet
        std::string full_filepath;          // root_dirpath + filename
        int64_t set_timeout_by_client;      // timeout value set by the client

        ClientHandler() : current_state(TransferState::WaitForTransfer), clientPort(-1), clientIP(""), direction(-1), block_number(0), tsize(-1), timeout(INITIALTIMEOUT), block_size(512), block_size_set(false), last_packet(false),
                          root_dirpath(""), udpSocket(-1), r_flag(false), overflow(""), attempts_to_resend(0), full_filepath(""), set_timeout_by_client(INITIALTIMEOUT) {}

        /**
         * @brief Handles a client connection, this is the function called by the server when received a message from new client.
         * @param receivedMessage The received message from the client.
         * @param bytesRead The number of bytes read from the received message.
         * @param clientAddr The address of the client.
         * @param root_dirpath The root directory path for file operations.
         */ 
        void handleClient(std::string receivedMessage, int bytesRead, sockaddr_in clientAddr, std::string root_dirpath);

        /**
         * @brief Handles a received TFTP packet.
         * @param packet The received TFTP packet.
         */
        void handlePacket(TFTPPacket *packet);

        /**
         * @brief Creates a UDP socket.
         * @return Created socket.
         */
        int createUdpSocket();

        /**
         * @brief Transfers a file to/from the client.
         * @return 0 if successful.
         */
        int transferFile();

        /**
         * @brief Writes data to the file when uploading to the server.
         * @param data The data to write.
         * @return 0 if successful.
         */
        int writeData(std::vector<char> data);

        /**
         * @brief Sets up the file for upload to the client - opens it for write, checks if everything is ok etc.
         * @return 0 if success.
         */
        int setupFileForUpload();

        /**
         * @brief Sets up the file for download from the client - opens it for read, checks if everything is ok etc.
         * @return 0 if success.
         */
        int setupFileForDownload();

        /**
         * @brief Handles sending data to the client.
         * @return 0 if successful.
         */
        int handleSendingData();
};
