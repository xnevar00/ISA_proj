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

int Client::parse_arguments(int argc, char* argv[], Client *client)
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
                client->hostname = optarg;
                break;
            case 'p':
                if (str_is_digits_only(optarg) == false) 
                {
                    return -1;
                } else 
                {
                    client->port = std::atoi(optarg);
                    if (client->port <= 0 || client->port > 65535) 
                    {
                        return -1;
                    }
                }
                break;
            case 'f':
                client->filepath = optarg;
                break;
            case 't':
                client->destFilepath = optarg;
                break;
            case '?':
                return -1;
            default:
                break;
        }
    }

    if (client->hostname.empty() || client->destFilepath.empty()) {
        return -1;
    }
    if ((argc < 5) || (argc > 9) || (argc%2 != 1)) {
        return -1;
    }
    std::cout << "Hostname: " << client->hostname << std::endl;
    std::cout << "Port: " << client->port << std::endl;
    std::cout << "Filepath: " << client->filepath << std::endl;
    std::cout << "Dest Filepath: " << client->destFilepath << std::endl;

    return 0;
}