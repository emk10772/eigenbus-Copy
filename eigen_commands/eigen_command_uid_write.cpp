#include "eigen_command_uid_write.h"


EigenCommandUIDWrite::EigenCommandUIDWrite(uint64_t UID, eigen_addr_t address)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_UID_WRITE), UID_(UID) {
}

std::string EigenCommandUIDWrite::packet() const{
    return strprintf("u%016llx)01,%02x,8675309", UID_, address_);
}

std::string EigenCommandUIDWrite::expected_response() const{
    return "|)01";
}

EigenCommand *EigenCommandUIDWrite::clone() const{
    return new EigenCommandUIDWrite(*this);
}
