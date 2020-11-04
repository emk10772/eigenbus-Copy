#ifndef EIGEN_COMMAND_UID_WRITE_H
#define EIGEN_COMMAND_UID_WRITE_H

#include "eigen_command.h"

class EigenCommandUIDWrite : public EigenCommand{
public:
    EigenCommandUIDWrite(uint64_t UID, eigen_addr_t address);

    std::string packet() override;
    std::string expected_response() override;

private:
    uint64_t UID_;
};

#endif // EIGEN_COMMAND_UID_WRITE_H
