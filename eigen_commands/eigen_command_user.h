#ifndef EIGEN_COMMAND_USER_H
#define EIGEN_COMMAND_USER_H

#include "eigen_command.h"

class EigenCommandUser : public EigenCommand{
public:
    EigenCommandUser(eigen_addr_t address, std::string command_str, std::string response = "");

    std::string packet() const override;
    std::string expected_response() const override;
    EigenCommand *clone() const override;
    EigenCommand *clone(eigen_addr_t addr) const override;

private:
    const std::string command_str_;
    const std::string response_;
};

#endif // EIGEN_COMMAND_USER_H
