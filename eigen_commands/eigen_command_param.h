#ifndef EIGEN_COMMAND_PARAM_H
#define EIGEN_COMMAND_PARAM_H

#include "eigen_command.h"
#include <stdint.h>
#include <string>

class EigenCommandParamRead : public EigenCommand{
public:
    EigenCommandParamRead(eigen_addr_t address, uint8_t id);

    std::string packet();
    std::string expected_response();

private:
    uint8_t id_;
};

class EigenCommandParamWrite : public EigenCommand{
public:
    EigenCommandParamWrite(eigen_addr_t address, uint8_t id, uint8_t value);
    EigenCommandParamWrite(eigen_addr_t address, uint8_t id, uint16_t value);
    EigenCommandParamWrite(eigen_addr_t address, uint8_t id, uint32_t value);
    EigenCommandParamWrite(eigen_addr_t address, uint8_t id, uint64_t value);
    EigenCommandParamWrite(eigen_addr_t address, uint8_t id, float value);
    EigenCommandParamWrite(eigen_addr_t address, uint8_t id, double value);
    EigenCommandParamWrite(eigen_addr_t address, uint8_t id, eigen_param_t value, uint8_t param_type);

    std::string packet();
    std::string expected_response();

private:
    eigen_param_t value_;
    uint8_t param_type_;
    uint8_t id_;
};

#endif // EIGEN_COMMAND_PARAM_H
