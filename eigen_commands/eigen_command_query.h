#ifndef EIGEN_COMMAND_QUERY_H
#define EIGEN_COMMAND_QUERY_H

#include "eigen_command.h"

class EigenCommandQuery : public EigenCommand{
public:
    EigenCommandQuery(eigen_addr_t address, uint8_t type);

    std::string packet() const override;
    std::string expected_response() const override;
    EigenCommand *clone() const override;

private:
    uint8_t type_;
};

#endif // EIGEN_COMMAND_QUERY_H
