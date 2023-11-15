#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include "../../include/client/client_class.hpp"


int main(int argc, char* argv[]) {
    Client *client = Client::getInstance();

    int ok = client->parse_arguments(argc, argv);
    if (ok != StatusCode::SUCCESS)
    {
        std::cout << "Chyba v zadani argumentu" << std::endl;
        return StatusCode::INVALID_ARGUMENTS;
    } else {
        std::cout << "Všechno je v pořádku" << std::endl;
    }
    ok = client->communicate();
    return ok;

}