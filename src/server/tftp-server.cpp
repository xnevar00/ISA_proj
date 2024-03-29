/**
 * @file tftp-server.cpp
 * @author Veronika Nevarilova (xnevar00)
 * @date 11/2023
 */


#include <iostream>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../../include/server/server_class.hpp"

int main(int argc, char* argv[]) {

    Server* server = Server::getInstance();

    int ok = server->parse_arguments(argc, argv, server);
    if (ok != StatusCode::SUCCESS)
    {
        OutputHandler::getInstance()->print_to_cout("Invalid format of arguments.");
        return ok;
    } else {
        OutputHandler::getInstance()->print_to_cout("Arguments ok.");
    }
    ok = server->listen(server);
    if (ok != StatusCode::SUCCESS)
    {
        OutputHandler::getInstance()->print_to_cout("Error creating socket.");
        return ok;
    }
    return StatusCode::SUCCESS;
}