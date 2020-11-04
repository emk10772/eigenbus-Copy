#ifndef EIGEN_RESPONSE_BOOTLOADER_H
#define EIGEN_RESPONSE_BOOTLOADER_H

#include "eigen_response.h"

class EigenResponseBootloader : public EigenResponse{
public:
    EigenResponseBootloader(std::string packet);

    bool update_module(ModuleShared mod) override;
    module_update_enum update_type() override;

    typedef enum{
        BOOTLOADER_READ,
        BOOTLOADER_ACK,
        BOOTLOADER_RESEND,
        BOOTLOADER_INVALID
    } btldr_response_t;

    btldr_response_t btldr_action();
    std::string data();

private:
    std::string data_;
    btldr_response_t action_;
};

#endif // EIGEN_RESPONSE_BOOTLOADER_H
