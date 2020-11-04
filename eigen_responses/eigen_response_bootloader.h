#ifndef EIGEN_RESPONSE_BOOTLOADER_H
#define EIGEN_RESPONSE_BOOTLOADER_H

#include "eigen_response.h"

class EigenResponseBootloader : public EigenResponse{
public:
    EigenResponseBootloader(std::string packet);

    bool update_module(ModuleShared mod) override;
    module_update_enum update_type() override;

private:
};

#endif // EIGEN_RESPONSE_BOOTLOADER_H
