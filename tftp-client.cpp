#include <iostream>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

struct client_config {
    int port = 69;
    std::string hostname = "";
    std::string filepath = "";
    std::string destFilepath = "";
    bool ok = true;
};

/*int create_udp_socket() {

}

int setup_udp_socket(int udpSocket, server_config server_cnfg){

}

int initialize_connection(server_config server_cnfg)
{

}

int listen(server_config server_cnfg)
{

}*/

client_config wrong_arguments() {
    client_config error;
    error.ok = false;
    std::cerr << "Wrong arguments passed! Usage: tftp-server [-p port] directory" << std::endl;
    return error;
}

bool str_is_digits_only(std::string str) {
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

client_config parse_arguments(int argc, char* argv[])
{
    client_config client_cnfg;
    bool port_set = false;
    bool filepath_set = false;

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
                client_cnfg.hostname = optarg;
                break;
            case 'p':
                if (str_is_digits_only(optarg) == false) 
                {
                    return wrong_arguments();
                } else 
                {
                    client_cnfg.port = std::atoi(optarg);
                    if (client_cnfg.port <= 0 || client_cnfg.port > 65535) 
                    {
                        return wrong_arguments();
                    }
                    port_set = true;
                }
                break;
            case 'f':
                client_cnfg.filepath = optarg;
                filepath_set = true;
                break;
            case 't':
                client_cnfg.destFilepath = optarg;
                break;
            case '?':
                return wrong_arguments();
            default:
                break;
        }
    }

    if (client_cnfg.hostname.empty() || client_cnfg.destFilepath.empty()) {
        return wrong_arguments();
    }
    if ((argc < 5) || (argc > 9) || (argc%2 != 1)) {
        return wrong_arguments();
    }
    std::cout << "Hostname: " << client_cnfg.hostname << std::endl;
    std::cout << "Port: " << client_cnfg.port << std::endl;
    std::cout << "Filepath: " << client_cnfg.filepath << std::endl;
    std::cout << "Dest Filepath: " << client_cnfg.destFilepath << std::endl;

    return client_cnfg;
}

int main(int argc, char* argv[]) {
    client_config client_cnfg = parse_arguments(argc, argv);
    if (client_cnfg.ok == false) {
        return -1;
    } else
    {
        std::cout << "Všechno je v pořádku" << std::endl;
    }
    return 0;
}