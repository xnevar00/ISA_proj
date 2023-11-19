/**
 * @file tftp-client.cpp
 * @author Veronika Nevarilova (xnevar00)
 * @date 11/2023
 */


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
        OutputHandler::getInstance()->print_to_cout("Invalid format of arguments.");
        return -1;
    } else {
        OutputHandler::getInstance()->print_to_cout("Arguments ok.");
    }
    ok = client->communicate();
    return ok;
}