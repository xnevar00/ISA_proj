#include "helper_functions.hpp"
#include <cerrno>

bool str_is_digits_only(std::string str) {
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

std::string getArgument(int startIndex, std::string receivedMessage)
{
    if (startIndex >= (int)receivedMessage.length() || startIndex < 0) {
        return "";
    }
    size_t nullByteIndex = receivedMessage.find('\0', startIndex);
    if (nullByteIndex != std::string::npos) {
        std::string result = receivedMessage.substr(startIndex, nullByteIndex - startIndex);
        return result;
    } else {
        return "";
    }
}

int getAnotherStartIndex(int startIndex, std::string receivedMessage)
{
    size_t nullByteIndex = receivedMessage.find('\0', startIndex);
    if (nullByteIndex != std::string::npos) {
        return nullByteIndex + 1;
    } else {
        return -1;
    }
}

std::string getSingleArgument(int startIndex, std::string receivedMessage)
{
    std::string argument = getArgument(startIndex, receivedMessage);
    if (argument == "")
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return "";
    }
    std::cout << "Argument: " << argument << std::endl;
    return argument;
}

int getOpcode(std::string receivedMessage)
{
    int opcode = -1;
    if(receivedMessage[0] >= '0' && receivedMessage[0] <= '9' && receivedMessage[1] >= '0' && receivedMessage[1] <= '9')
    {
        std::string opcodeStr = receivedMessage.substr(0, 2);
        opcode = std::stoi(opcodeStr);
    } else{
        return -1;
    }
    return opcode;
}

int setOption(int *option, int *optionIndex, std::string receivedMessage)
{
    *optionIndex = getAnotherStartIndex(*optionIndex, receivedMessage);
    std::string option_str = getSingleArgument(*optionIndex, receivedMessage);
    if (option_str == "" || !str_is_digits_only(option_str))
    {
        return -1;
    } else
    {
        *option = std::stoi(option_str);
        return 0;
    }
}

std::vector<char> intToBytes(unsigned short value) {

    std::vector<char> bytes(2);
    bytes[0] = static_cast<char>((value >> 8) & 0xFF); // První byte (nejstarší bity)
    bytes[1] = static_cast<char>(value & 0xFF);        // Druhý byte (mladší bity)
    return bytes;
}