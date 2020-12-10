#include "eigen_response_broadcast.h"

#include <string>


EigenResponseBroadcast::EigenResponseBroadcast(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_BOOTLOADER) {

}

EigenUpdate *EigenResponseBroadcast::update_module(ModuleShared mod, uint64_t latency){
    uint64_t t_adjusted = t_received() - (latency / 2);
    switch(packet_[0]){
    case 's': {
        //Increase the sync start time
        break;
    }

    case 'q': {
        //Update broadcast reg
        break;
    }

    case 'p': {
        //Update period reg
        break;
    }

    case 'r': { //Reset sync
        //Reset our sync time
        mod->t_broadcast_sync_start = t_received() - latency;
        mod->t_broadcast_offset += latency / 2;
        mod->broadcast_sync_count = 1;

        //Start taking the average offset
        break;
    }

    case 't': {
        //If we have not reached the count yet, send another command
        //Calculate the difference from what we expect the time to be
        mod->t_broadcast_offset += latency / 2;
        mod->broadcast_sync_count++;
        break;
    }

    case 'g': {
        mod->broadcast_reg;
        break;
    }

    }

    return nullptr;
}


bool EigenResponseBroadcast::isSpontaneous(){
    return true;
}

