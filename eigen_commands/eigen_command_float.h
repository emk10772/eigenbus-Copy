#ifndef EIGEN_COMMAND_FLOAT_H
#define EIGEN_COMMAND_FLOAT_H

#include "eigen_command.h"
#include <stdint.h>
#include <string>

/* EigenCommandFloat:
 * An easy base class for commands that write a double/float value to the Eigenbus.
 */
class EigenCommandFloat : public EigenCommand{
public:
    EigenCommandFloat(eigen_addr_t address, std::string command, std::string response, double value, command_t command_type);

    std::string packet() const override;
    std::string expected_response() const override;
    EigenCommand *clone() const override;
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
