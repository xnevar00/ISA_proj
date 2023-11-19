/**
 * @file output_handler.hpp
 * @author Veronika Nevarilova
 * @date 11/2023
 */

#include <string>
#include <mutex>

/**
 * @class OutputHandler
 * @brief A singleton class that provides thread-safe output operations.
 *
 */
class OutputHandler {

protected:
    OutputHandler(){}
    static OutputHandler* outputHandler_;

public:
    OutputHandler(OutputHandler &other) = delete;
    void operator=(const OutputHandler&) = delete;

    /**
     * @brief Get the single instance of the OutputHandler class.
     * @return A pointer to the single instance of the class.
     */
    static OutputHandler* getInstance();

    /**
     * @brief Print a message to standard output in a thread-safe manner.
     * @param message The message to print.
     */
    void print_to_cout(const std::string& message);

    /**
     * @brief Print a message to standard error in a thread-safe manner.
     * @param message The message to print.
     */
    void print_to_cerr(const std::string& message);

    // mutexes used for thread-safe printing
    std::mutex cout_mutex;
    std::mutex cerr_mutex;

};