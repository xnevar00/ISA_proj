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
    /*std::streambuf* orig_stdout = std::cout.rdbuf();
    std::streambuf* orig_stderr = std::cerr.rdbuf();

    // Přesměrování stdout do souboru "output.txt"
    std::ofstream file("outputserver.txt");
    std::cout.rdbuf(file.rdbuf()); */


    Server* server = Server::getInstance();

    int ok = server->parse_arguments(argc, argv, server);
    if (ok != StatusCode::SUCCESS)
    {
        std::cout << "Chyba v zadani argumentu" << std::endl;
        return ok;
    } else {
        std::cout << "Všechno je v pořádku" << std::endl;
    }
    ok = server->listen(server);
    if (ok != StatusCode::SUCCESS)
    {
        std::cout << "Chyba pri inicializaci socketu" << std::endl;
        return ok;
    }
    return StatusCode::SUCCESS;
}