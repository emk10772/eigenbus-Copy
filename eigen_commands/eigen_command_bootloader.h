#ifndef EIGEN_COMMAND_BOOTLOADER_H
#define EIGEN_COMMAND_BOOTLOADER_H

#include "eigen_command.h"

class EigenCommandBootloader : public EigenCommand{
public:
    typedef enum{
        BOOTLOADER_START,
        BOOTLOADER_RESEND,
        BOOTLOADER_ACK
    } btldr_op_t;

    EigenCommandBootloader(eigen_addr_t address, btldr_op_t action);
    EigenCommandBootloader(eigen_addr_t address, std::string msg);

    std::string packet() override;
    std::string expected_response() override;

private:
    btldr_op_t action_;
    std::string msg_;
};

#endif // EIGEN_COMMAND_BOOTLOADER_H
