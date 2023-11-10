#include <iostream>
#include <getopt.h>
#include "../../helper_functions.hpp"

class Server
{

protected:
    Server(){}

    static Server* server_;

public:
    Server(Server &other) = delete;
    
    int port;
    std::string root_dirpath;


    void operator=(const Server &) = delete;

    static Server *getInstance();
    int parse_arguments(int argc, char* argv[], Server *server);
};

