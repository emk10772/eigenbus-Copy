#ifndef EIGEN_RESPONSE_BOOTLOADER_H
#define EIGEN_RESPONSE_BOOTLOADER_H

#include "eigen_response.h"

class EigenResponseBootloader : public EigenResponse{
public:
    EigenResponseBootloader(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod, uint64_t latency) override;

    typedef enum{
        BOOTLOADER_READ,
        BOOTLOADER_ACK,
        BOOTLOADER_RESEND,
        BOOTLOADER_INVALID
    } btldr_response_t;

    btldr_response_t btldr_action();
    std::string data();
    bool isSpontaneous() override;

private:
    std::string data_;
    btldr_response_t action_;
};

#endif // EIGEN_RESPONSE_BOOTLOADER_H
