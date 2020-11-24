#ifndef EIGEN_UPDATE_H
#define EIGEN_UPDATE_H

#include "eigen_utils.h"
#include "eigen_module.h"

class EigenUpdate{
public:
    typedef enum {
        MODULE_TOUCHED,
        MODULE_ERROR,
        MODULE_ADDED,
        MODULE_STALE,
        MODULE_DOWNSTREAM,
        MODULE_REMOVED,
        MODULE_BTLDR_PROGRESS,
        MODULE_BTLDR_END,
        MODULE_POS_UPDATE,
        MODULE_VEL_UPDATE,
        MODULE_EFF_UPDATE,
        MODULE_PARAM_READ,
        MODULE_PARAM_ADD,
        MODULE_PARAM_WRITE,
        MODULE_PARAM_ERR,
        MODULE_MAIL_READ,
        MODULE_MAIL_ADD,
        MODULE_MAIL_WRITE,
        MODULE_MAIL_ERR
    } update_t;

    EigenUpdate(eigen_addr_t address, update_t type, ModuleConst mod = nullptr, uint8_t arg = 0, std::string data = "N/A");
    ~EigenUpdate();

    update_t type();
    eigen_addr_t address();

    uint8_t arg();
    std::string data();
    ModuleConst module();

private:
    const eigen_addr_t address_;
    const update_t type_;
    const ModuleConst mod_;
    const uint8_t arg_;
    const std::string data_;
};

#endif // EIGEN_UPDATE_H
