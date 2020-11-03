#ifndef EIGEN_COMMAND_FLOAT_H
#define EIGEN_COMMAND_FLOAT_H

#include "eigen_command.h"
#include <stdint.h>
#include <string>

class EigenCommandFloat : public EigenCommand{
public:
    EigenCommandFloat(eigen_addr_t address, std::string command, std::string response, double value);

    std::string packet();
    std::string expected_response();
    packet_type type();

protected:
    const std::string command_;
    const std::string response_;
    const double value_;
};

class EigenCommandPosition : public EigenCommandFloat{
public:
    EigenCommandPosition(eigen_addr_t address, double value);
};

class EigenCommandVelocity : public EigenCommandFloat{
public:
    EigenCommandVelocity(eigen_addr_t address, double value);
};

class EigenCommandEffort : public EigenCommandFloat{
public:
    EigenCommandEffort(eigen_addr_t address, double value);
};

#endif // EIGEN_COMMAND_FLOAT_H
