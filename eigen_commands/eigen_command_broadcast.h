#ifndef EIGEN_COMMAND_BROADCAST_H
#define EIGEN_COMMAND_BROADCAST_H

#include "eigen_command.h"

class EigenCommandBroadcast : public EigenCommand{
public:
    typedef enum{
        BOOTLOADER_START,
        BOOTLOADER_RESEND,
        BOOTLOADER_ACK
    } btldr_op_t;

    EigenCommandBroadcast(eigen_addr_t address, btldr_op_t action, uint8_t mode = 0, std::string msg = "");
    EigenCommandBroadcast(eigen_addr_t address, std::string msg);
    EigenCommandBroadcast(eigen_addr_t address, uint8_t mode, std::string msg);

    std::string packet() const override;
    std::string expected_response() const override;
    EigenCommand *clone() const override;

    std::string msg();
    uint8_t mode();
    btldr_op_t action();

private:
    btldr_op_t action_;
    std::string msg_;
    uint8_t mode_;
};

#endif // EIGEN_COMMAND_BROADCAST_H
