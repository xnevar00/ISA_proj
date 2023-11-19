/**
 * @file client_class.hpp
 * @author Veronika Nevarilova (xnevar00)
 * @date 11/2023
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include <vector>
#include <signal.h>
#include <csignal>
#include <atomic>
#include "../../include/packet/tftp_packet_class.hpp"

/**
 * @brief Function used to handle the SIGINT signal.
 * @param signal received signal.
 */
void signalHandlerClient(int signal);


/**
 * @brief Sets up the signal handler for SIGINT.
 */
void setupSignalHandlerClient();


/**
 * @class Client
 * @brief A class that represents a TFTP client as a singleton
 *
 */
class Client
{
    protected:
        Client(){}
        static Client* client_;

        int port;                           // initial port set by the user
        std::string hostname;               // hostname set by the user
        std::string filepath;               // filepath set by the user
        std::string destFilepath;           // destination filepath set by the user
        int udpSocket;                      // UDP socket used for communication
        sockaddr_in serverAddr;             // address of the server on which responds to the first message
        bool last_packet;                   // flag that indicates whether the last packet was sent or received
        unsigned short int block_number;    // block number of the packet
        int block_size;                     // block size of the packet
        int timeout;                        // timeout of the packet
        int tsize;                          // tsize of the packet
        Direction direction;                // upload = wrq, download = rrq
        std::ofstream file;                 // file to be read from or written to
        TransferState current_state;        // state of the transfer
        int serverPort;                     // port of the server - used for printing to stderr
        std::string serverIP;               // IP address of the server - used for printing to stderr
        int clientPort;                     // port of the client - used for printing to stderr
        std::string mode;                   // set mode of the transfer
        bool r_flag;                        // flag to indicate whether in the last packet on the last index was a '\r' character (used in netascii mode)
        std::string overflow;               // string that is used to store the overflow of the last packet (used in netascii mode)
        int attempts_to_resend;             // number of attempts to resend the last packet
        std::vector<char> last_data;        // last data sent - used for resending in case of timeout

        /**
         * @brief Create a UDP socket.
         * @return created socket.
         */
        int createUdpSocket();

        /**
         * @brief Transfer data to/from the server.
         * @return 0 if successful.
         */
        int transferData();

        /**
         * @brief Send the first message (RRQ or WRQ) to the server.
         * @return 0 if successful.
         */
        int sendBroadcastMessage();

        /**
         * @brief Update the accepted options based on the options the server agreed on.
         * @param packet The received packet.
         */
        void updateAcceptedOptions(TFTPPacket *packet);

        /**
         * @brief Transfer a file to/from the server.
         * @return 0 if successful.
         */
        int transferFile();

        /**
         * @brief Setup the file for download from the server.
         * @return 0 if successful.
         */
        int setupFileForDownload();

        /**
         * @brief Handle sending data to the server.
         * @return 0 if successful.
         */
        int handleSendingData();

    public:
        Client(Client &other) = delete;
        void operator=(const Client &) = delete;

        /**
         * @brief Get the single instance of the Client class.
         * @return A pointer to the single instance of the Client class.
         */
        static Client *getInstance();
        
        /**
         * @brief Parse the command line arguments.
         * @param argc The number of command line arguments.
         * @param argv The command line arguments.
         * @return 0 if successful.
         */
        int parse_arguments(int argc, char* argv[]);

        /**
         * @brief Communicate with the server.
         * @return 0 if successful.
         */
        int communicate();
};

