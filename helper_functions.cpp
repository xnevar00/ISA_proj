#include "helper_functions.hpp"

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
    if (startIndex >= (int)receivedMessage.length() || startIndex == -1) {
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

int getPairArgument(int optionIndex, std::string receivedMessage, Session *session)
{
    std::string option = getArgument(optionIndex, receivedMessage);
    optionIndex = getAnotherStartIndex(optionIndex, receivedMessage);
    if (optionIndex == -1 || option == "")
    {
        return -1;
    }
    if (option == "blksize" || option == "timeout" || option == "tsize")
    {
        if (option == "blksize")
        {
            if (session->blksize_option == true)
            {
                return -1;
            }
            session->blksize_option = true;
        } else if(option == "timeout")
        {
            if(session->timeout_option == true)
            {
                return -1;
            }
            session->timeout_option = true;
        } else if (option == "tsize") 
        {
            if(session->tsize_option == true)
            {
                return -1;
            }
            session->tsize_option = true;
        
        } else
        {
            return -1;
        }
        std::string option_value = getArgument(optionIndex, receivedMessage);
        if (option_value == "" || !str_is_digits_only(option_value))
        {
            return -1;
        }
        if (option == "blksize")
        {
            session->blksize = std::stoi(option_value);
            std::cout << "Blksize: " << option_value << std::endl;
        } else if (option == "timeout")
        {
            session->timeout = std::stoi(option_value);
            std::cout << "Timeout: " << option_value << std::endl;
        } else if (option == "tsize")
        {
            session->tsize = std::stoi(option_value);
            std::cout << "Timeout: " << option_value << std::endl;
        }
    } else
    {
        return -1;
    }
    return optionIndex;
}

void setMode(std::string mode, Session *session)
{
    if (mode == "netascii")
    {
        session->mode = Mode::NETASCII;
    } else if (mode == "octet")
    {
        session->mode = Mode::OCTET;
    } else
    {
        //TODO error packet
        std::cout << "CHYBA" << std::endl;
        return;
    }
}

int setupFileForUpload(Session *session)
{
    std::string full_filepath = session->root_dirpath + "/" + session->filename;
    session->file.open(full_filepath, std::ios::out | std::ios::app | std::ios::binary);

    if (!(session->file).is_open()) {
        std::cerr << "Error opening the file" << std::endl;
        return -1;
    }
    std::cout << "File opened" << std::endl;
    return 0;
}