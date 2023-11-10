#include <iostream>
#include <getopt.h>
#include "../../helper_functions.hpp"

class Client
{

protected:
    Client(){}

    static Client* client_;

public:
    Client(Client &other) = delete;
    
    int port;
    std::string hostname;
    std::string filepath;
    std::string destFilepath;

    void operator=(const Client &) = delete;

    static Client *getInstance();
    int parse_arguments(int argc, char* argv[], Client *client);
};

