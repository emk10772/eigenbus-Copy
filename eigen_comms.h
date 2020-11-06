#ifndef EIGEN_COMMS_H
#define EIGEN_COMMS_H

#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <mutex>
#include <exception>
#include <memory>

#include "eigen_utils.h"
#include "eigen_update.h"
#include "eigen_module.h"
#include "eigen_packet_filter.h"
#include "eigen_packet_parser.h"
#include "eigen_commands/eigen_commands.h"
#include "eigen_responses/eigen_response.h"

typedef struct eigen_config_struct{
    uint8_t poll_encoder_status;
    uint8_t poll_module_position;
    uint8_t poll_module_velocity;
    uint8_t poll_module_effort;
    uint8_t raw_packet_en;
    uint8_t max_retries;
} eigen_config;

typedef struct eigen_stats_struct{
    uint64_t uptime_ms;
    uint64_t sent_packets;
    uint64_t successful_packets;
    uint64_t dropped_packets;
    uint64_t unrequested_packets;
    uint64_t retried_packets;
    uint64_t frame_time_ms;
    std::string last_dropped_packet;
} eigen_stats;

//Interface running functions
void start_eigen_comms(uint16_t (*read)(uint8_t *buf, uint16_t max_len, int t_wait_ms),
                       void (*write)(uint8_t *buf, uint16_t len));
void clean_eigen_comms();
int service_eigen_comms();
bool get_poll_enabled();
void set_poll_enabled(bool state);
EigenUpdate *get_module_update();
uint64_t current_time_ms();
bool is_bootloader_active();
bool is_bootloader_finished();

//Config Interface
void set_eigen_config(eigen_config config);
eigen_config get_eigen_config();

//Command Interface
void add_command(EigenCommand *command);
void clear_commands();

//Packet tracking interface
raw_packet *get_raw_packet();
eigen_stats get_eigen_stats(); //TODO: Change to ptr?

ModuleConst get_module(uint8_t address);
ModuleConst get_module_by_index(uint8_t index);
uint8_t num_modules();
bool list_updated();

#endif


