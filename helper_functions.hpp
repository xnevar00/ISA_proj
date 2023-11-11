#include <iostream>

enum StatusCode {
    SUCCESS = 0,
    INVALID_ARGUMENTS = -1,
    SOCKET_ERROR = -2,
    CONNECTION_ERROR = -3,
    PACKET_ERROR = -4,
};

enum class ServerState {
    Idle,
    ReceiveWriteRequest,
    SendAcknowledgement,
    ReceiveAcknowledgement,
    SendData,
    WaitForAcknowledgement
};

bool str_is_digits_only(std::string str);