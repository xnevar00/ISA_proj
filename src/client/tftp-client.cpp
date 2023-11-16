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
    /*std::streambuf* orig_stdout = std::cout.rdbuf();
    std::streambuf* orig_stderr = std::cerr.rdbuf();

    // Přesměrování stdout do souboru "output.txt"
    std::ofstream file("outputclient.txt");
    std::cout.rdbuf(file.rdbuf());*/

    Client *client = Client::getInstance();

    int ok = client->parse_arguments(argc, argv);
    if (ok == -1)
    {
        std::cout << "Invalid format of arguments." << std::endl;
        return -1;
    } else {
        std::cout << "Arguments ok." << std::endl;
    }
    ok = client->communicate();
    return ok;

}