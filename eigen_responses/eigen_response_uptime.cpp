#include "eigen_response_uptime.h"


EigenResponseUptime::EigenResponseUptime(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EIGEN_UPTIME) {
    time = 0;
}

EigenUpdate *EigenResponseUptime::update_module(ModuleShared mod, uint64_t latency){
    time = stoul(packet_, nullptr, EIGENBUS_BASE);
    mod->t_last_uptime = time;

    return nullptr;
}

bool EigenResponseUptime::isSpontaneous(){
    return true;
}


