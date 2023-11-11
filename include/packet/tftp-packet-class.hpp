#include <memory>
//#include "../../include/server/server_class.hpp"
#include "../../helper_functions.hpp"

enum Opcode {
    RRQ = 1,
    WRQ = 2,
    DATA = 3,
    ACK = 4,
    ERROR = 5
};

class TFTPPacket {
public:
    virtual void parse(Session* session) const = 0;
    virtual ~TFTPPacket() {};
};

class RRQPacket : public TFTPPacket {
public:
    void parse(Session* session) const override;
};

class WRQPacket : public TFTPPacket {
public:
    void parse(Session* session) const override;
};

class DATAPacket : public TFTPPacket {
public:
    void parse(Session* session) const override;
};

class ACKPacket : public TFTPPacket {
public:
    void parse(Session* session) const override;
};

class ERRORPacket : public TFTPPacket {
public:
    void parse(Session* session) const override;
};