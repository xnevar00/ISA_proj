#include <iostream>

enum StatusCode {
    SUCCESS = 0,
    INVALID_ARGUMENTS = -1,
    SOCKET_ERROR = -2,
    CONNECTION_ERROR = -3,
};

bool str_is_digits_only(std::string str);