#include <string>
#include <mutex>

class OutputHandler {

protected:
    OutputHandler(){}
    static OutputHandler* outputHandler_;

public:
    OutputHandler(OutputHandler &other) = delete;
    void operator=(const OutputHandler&) = delete;
    static OutputHandler* getInstance();

    void print_to_cout(const std::string& message);
    void print_to_cerr(const std::string& message);
    std::mutex cout_mutex;
    std::mutex cerr_mutex;

};