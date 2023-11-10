#include <iostream>
#include "../../include/server/server_class.hpp"

Server* Server::server_ = nullptr;;

/**
 * Static methods should be defined outside the class.
 */
Server *Server::getInstance()
{
    /**
     * This is a safer way to create an instance. instance = new Singleton is
     * dangeruous in case two instance threads wants to access at the same time
     */
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
                    //server_cnfg.port = std::atoi(optarg);
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

    std::cout << "Port: " << server->port << std::endl;
    std::cout << "Directory: " << server->root_dirpath << std::endl;

    return 0;
}