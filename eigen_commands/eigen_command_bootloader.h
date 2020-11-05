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

    EigenCommandBootloader(eigen_addr_t address, btldr_op_t action, uint8_t mode = 0, std::string msg = "");
    EigenCommandBootloader(eigen_addr_t address, std::string msg);
    EigenCommandBootloader(eigen_addr_t address, uint8_t mode, std::string msg);

    std::string packet() override;
    std::string expected_response() override;

    std::string msg();
    uint8_t mode();

private:
    btldr_op_t action_;
    std::string msg_;
    uint8_t mode_;
};

#endif // EIGEN_COMMAND_BOOTLOADER_H
