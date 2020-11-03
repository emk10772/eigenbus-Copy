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
    EigenCommandSimple(eigen_addr_t address, std::string command, std::string response);

    std::string packet();
    std::string expected_response();
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

//The eigenbus technically supports more parameters for this command,
//but those are not used in practice
class EigenCommandZero : public EigenCommandSimple{
public:
    EigenCommandZero(eigen_addr_t address);
};


#endif // EIGEN_COMMAND_SIMPLE_H
