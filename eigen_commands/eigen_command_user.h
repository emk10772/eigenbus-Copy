#ifndef EIGEN_COMMAND_USER_H
#define EIGEN_COMMAND_USER_H

#include "eigen_command.h"

class EigenCommandUser : public EigenCommand{
public:
    EigenCommandUser(eigen_addr_t address, std::string command, std::string response = "");

    std::string packet() override;
    std::string expected_response() override;

private:
    const std::string command_;
    const std::string response_;
};

#endif // EIGEN_COMMAND_USER_H
