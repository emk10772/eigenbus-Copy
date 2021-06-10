#ifndef EIGEN_COMMAND_PARAM_H
#define EIGEN_COMMAND_PARAM_H

#include "eigen_command.h"
#include <stdint.h>
#include <string>

class EigenCommandParamRead : public EigenCommand{
public:
    EigenCommandParamRead(eigen_addr_t address, uint8_t id);

    std::string packet() const override;
    std::string expected_response() const override;
    EigenCommand *clone() const override;
    EigenCommand *clone(eigen_addr_t addr) const override;

private:
    uint8_t id_;
};

class EigenCommandParamWrite : public EigenCommand{
public:
    EigenCommandParamWrite(eigen_addr_t address, uint8_t id, EigenParameter param);

    std::string packet() const override;
    std::string expected_response() const override;
    EigenCommand *clone() const override;
    EigenCommand *clone(eigen_addr_t addr) const override;

private:
    EigenParameter param_;
    uint8_t id_;
};

#endif // EIGEN_COMMAND_PARAM_H
