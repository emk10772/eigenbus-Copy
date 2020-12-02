#ifndef EIGEN_COMMAND_SIMPLE_H
#define EIGEN_COMMAND_SIMPLE_H

#include "eigen_command.h"
#include <stdint.h>
#include <string>

/* EigenCommandSimple:
 *  A base class for simple commands, i.e. commands that have no other parameters.
 */
class EigenCommandSimple : public EigenCommand{
public:
    EigenCommandSimple(eigen_addr_t address, std::string command, std::string response, packet_type type, command_t command_type);

    std::string packet() const override;
    std::string expected_response() const override;
    EigenCommand *clone() const override;
    packet_type type();

protected:
    const std::string command_;
    const std::string response_;
};

class EigenCommandRun : public EigenCommandSimple{
public:
    EigenCommandRun(eigen_addr_t address);
};

class EigenCommandTopology : public EigenCommandSimple{
public:
    EigenCommandTopology(eigen_addr_t address);
};

class EigenCommandZero : public EigenCommandSimple{
public:
    EigenCommandZero(eigen_addr_t address, std::string arg = "");
};

class EigenCommandEcho : public EigenCommandSimple{
public:
    EigenCommandEcho(eigen_addr_t address, std::string echo_string);
};


#endif // EIGEN_COMMAND_SIMPLE_H
