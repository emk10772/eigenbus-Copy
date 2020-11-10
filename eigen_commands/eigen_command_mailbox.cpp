#include "eigen_command_mailbox.h"

EigenCommandMailboxRead::EigenCommandMailboxRead(eigen_addr_t address, uint8_t id)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_MAIL_READ){
    id_ = id;
}

std::string EigenCommandMailboxRead::packet() const{
    return strprintf("%02x[%02x", address_, id_);
}

std::string EigenCommandMailboxRead::expected_response() const{
    return strprintf("|[%02X", id_);
}

EigenCommand *EigenCommandMailboxRead::clone() const{
    return new EigenCommandMailboxRead(*this);
}



std::string EigenCommandMailboxWrite::packet() const{
    return strprintf("%02X]%02X,%s,8675309", address_, id_, data_.print().c_str());
}

std::string EigenCommandMailboxWrite::expected_response() const{
    if(!data_.valid()) return "";

    return strprintf("|]%02X", id_);
}

EigenCommandMailboxWrite::EigenCommandMailboxWrite(eigen_addr_t address, uint8_t id, EigenMailbox data)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_MAIL_WRITE), data_(data){
    id_ = id;
}

EigenCommand *EigenCommandMailboxWrite::clone() const{
    return new EigenCommandMailboxWrite(*this);
}
