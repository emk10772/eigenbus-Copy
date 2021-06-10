#include "eigen_command_query.h"


EigenCommandQuery::EigenCommandQuery(eigen_addr_t address, uint8_t type)
    : EigenCommand(address, EIGEN_PACKET_POLL, EIGEN_CMD_QUERY), type_(type) {
}

std::string EigenCommandQuery::packet() const{
    return strprintf("%02xQ%02x", address_, type_);
}

std::string EigenCommandQuery::expected_response() const{
    switch(type_){
    case EIGEN_POLL_LOCATION: 
        //Expecting a response of type L
        return "L";
    case EIGEN_POLL_VELOCITY:
        //Expecting a response of type V
        return "V";
    case EIGEN_POLL_EFFORT:
        //Expecting a response of type I
        return "I";
    case EIGEN_POLL_ENC_STATUS:
        //Expecting a response of type N
        return "N";
    default:
        //Unsupported, not expecting a response
        return "";
    }
}

EigenCommand *EigenCommandQuery::clone() const{
    return new EigenCommandQuery(*this);
}

EigenCommand *EigenCommandQuery::clone(eigen_addr_t addr) const{
    return new EigenCommandQuery(addr, type_);
}
