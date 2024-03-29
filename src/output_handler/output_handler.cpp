/**
 * @file output_handler.cpp
 * @author Veronika Nevarilova (xnevar00)
 * @date 11/2023
 */


#include "../../include/output_handler/output_handler.hpp"

OutputHandler* OutputHandler::outputHandler_ = nullptr;;

OutputHandler *OutputHandler::getInstance()
{
    if(outputHandler_== nullptr){
        outputHandler_ = new OutputHandler();
    }
    return outputHandler_;
}

void OutputHandler::print_to_cout(const std::string& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << message << std::endl;
}

void OutputHandler::print_to_cerr(const std::string& message) {
        std::lock_guard<std::mutex> lock(cerr_mutex);
        std::cerr << message << std::endl;
}
