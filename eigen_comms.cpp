#include "eigen_comms.h"
#include <stdlib.h>
#include <ctype.h>
#include <chrono>
#include <algorithm>
#include <deque>
#include <stdio.h>
#include <string.h>
#include <cmath>

#ifdef EIGEN_BTLDR_SUPPORT
#include "cybootloaderutils\cybtldr_api2.h"
#include "cybootloaderutils\cybtldr_api.h"
#endif

#define IN_BUFFER_SIZE          (255)

#define ADDR_PARAM              (1)
#define TYPE_PARAM              (2)

//Private Variables
static uint16_t (*read_data)(uint8_t *buf, uint16_t max_len, int t_wait_ms);
static void (*write_data)(uint8_t *buf, uint16_t len);

static uint64_t t_last_poll = 0;
static uint64_t t_last_update_poll = 0;
static uint64_t t_last_status_poll = 0;
static uint64_t t_init = 0;

//static uint64_t sent_packets = 0;
//static uint64_t dropped_packets = 0;
//static uint64_t successful_packets = 0;
//static uint64_t unrequested_packets = 0;
//static uint64_t retried_packets = 0;
static uint64_t frame_time = 0;
//static std::string last_dropped = "";

//Module data structure
//static std::map<uint8_t, Module *> module_map;
static std::vector<std::shared_ptr<Module>> module_list;
std::mutex module_list_mutex;

//Interface deques and mutexes for thread safety
static std::deque<module_update *> update_list;
static std::mutex update_mutex;
static std::deque<EigenCommand *> cmd_list;
static std::mutex cmd_mutex;

static EigenPacketTracker *packetTracker;
static EigenPacketParser *packetParser;

static std::deque<std::string> packet_queue;

static bool list_update;
static bool enabled;
static eigen_config communication_config;


/* Private forward declarations */
void clear_module_list();
EigenCommand *get_command();
void add_module_update(uint8_t addr, update_enum type);
void add_module_update(uint8_t addr, update_enum type, uint8_t arg);
uint8_t generate_node_address();

//Module list interface functions
ModuleShared add_module(uint8_t address);
ModuleShared get_module_shared(uint8_t address);

void write_packet(uint8_t *buf, uint8_t len){
    static uint8_t s[256];

    if(len == 0) return;

    uint8_t crc = crc_8_ccitt(buf, len);

    int count = std::snprintf((char *)s, 256, "%s:%02X\n", buf, crc);
    (*write_data)(s, count);
}

uint64_t current_time_ms() {
    auto current_time = std::chrono::system_clock::now();
    auto epoch = current_time.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
    return milliseconds.count();
}

eigen_stats get_eigen_stats(){
    eigen_stats stats;
    stats.uptime_ms = current_time_ms() - t_init;
    stats.sent_packets = sent_packets;
    stats.successful_packets = successful_packets;
    stats.dropped_packets = dropped_packets;
    stats.unrequested_packets = unrequested_packets;
    stats.retried_packets = retried_packets;
    stats.frame_time_ms = frame_time;
    stats.last_dropped_packet = last_dropped;
    return stats;
}

void start_eigen_comms(uint16_t (*read)(uint8_t *buf, uint16_t max_len, int t_wait_ms),
                       void (*write)(uint8_t *buf, uint16_t len)) {
    read_data = read;
    write_data = write;
    list_update = true;
    enabled = false;

    communication_config.poll_encoder_status = EIGEN_DISABLED;
    communication_config.poll_module_position = EIGEN_DISABLED;
    communication_config.raw_packet_en = EIGEN_ENABLED;
    communication_config.max_retries = 2;

    t_init = current_time_ms();
    //null_mod = new Module(0xFF);
    srand(t_init);

#ifdef EIGEN_BTLDR_SUPPORT
    bootloader_init();
#endif

    packetTracker = new EigenPacketTracker();
    packetParser = new EigenPacketParser();

}

void clean_eigen_comms() {
    //TODO: Free each module, clean up the map

    clear_module_list();
    delete packetParser;
    delete packetTracker;
    //delete null_mod;
}

void clear_module_list(){
    module_list.clear();
    /*auto it = module_list.begin();
    while(it != module_list.end()){
        delete *it;
        it = module_list.erase(it);
    }*/
}

void clean_module_list(){
#ifndef MODULE_TEST
    auto mod_it = module_list.begin();
    while(mod_it != module_list.end()){
        //If we haven't heard from the module in a while, remove it
        if(current_time_ms() - (*mod_it)->t_last_update > 2*UPDATE_TOPOLOGY_PERIOD){
            add_module_update((*mod_it)->get_address(), MODULE_REMOVED);
            //(*mod_it)->stale = true;
            mod_it = module_list.erase(mod_it);
            //++mod_it;
        } else {
            ++mod_it;
        }
    }
#endif
}

void add_module_update(uint8_t addr, update_enum type){
    std::lock_guard<std::mutex> lock(update_mutex);

    module_update *update = new module_update;
    update->index = addr;
    update->update_type = type;
    update->arg = 0;
    update->data = "";
    update_list.push_back(update);
}

void add_module_update(uint8_t addr, update_enum type, uint8_t arg){
    std::lock_guard<std::mutex> lock(update_mutex);

    module_update *update = new module_update;
    update->index = addr;
    update->update_type = type;
    update->arg = arg;
    update->data = "";
    update_list.push_back(update);
}

void add_module_update(uint8_t addr, update_enum type, std::string data){
    std::lock_guard<std::mutex> lock(update_mutex);

    module_update *update = new module_update;
    update->index = addr;
    update->update_type = type;
    update->arg = 0;
    update->data = std::string(data);
    update_list.push_back(update);
}

module_update *get_module_update(){
    module_update *retval;
    std::lock_guard<std::mutex> lock(update_mutex);

    if(update_list.size() > 0){
        retval = update_list.front();
        update_list.pop_front();
    } else {
        return NULL;
    }

    return retval;
}

uint8_t generate_node_address(){
    //Not a very efficient implementation, but should be fine because we will use this very rarely
    std::set<uint8_t> occupied_addrs;

    //TODO: Add a check for too many devices
    for(auto mod : module_list){
        occupied_addrs.insert(mod->get_address());
    }

    //Randomly try to find a free address
    uint8_t addr = rand() % 0xFF;
    while(occupied_addrs.count(addr) != 0){
        addr = rand() % 0xFF;
    }

    //TODO: Check this
    return addr;
}

void process_packet(uint8_t *buffer, uint8_t len) {
    uint8_t addr = 0;
    uint8_t *ptr = buffer;
    
    if (len > 3 && buffer[0] == '.' &&
        isxdigit(buffer[1]) && isxdigit(buffer[2])) { //Feedback messages have periods

        uint8_t pkt_valid = 1;
        //Check for a valid address
        addr = strtol((char *)buffer+1, (char **)&ptr, 16);
        if (ptr - buffer != 3){
            pkt_valid = 0;
        }

        //Check the checksum. Should start at len - 3 if this is a valid packet
        if(buffer[len-3] == ':'){
            uint8_t chk_read = strtol((char *)buffer + len - 2, NULL, 16);
            uint8_t chk_calc = crc_8_ccitt(buffer, len-3);
            if(chk_read != chk_calc){
                pkt_valid = 0;
            } else {
                buffer[len - 3] = 0;
            }
        } else {
            pkt_valid = 0;
        }

        if(pkt_valid == 0){

#ifdef EIGEN_BTLDR_SUPPORT
            if(bootloader_active)
                request_resend_bootload(bootloader_target_addr, "BD-PKT");
#endif
            return;
        }

        //If the address is valid add it to the list
        ModuleShared module = add_module(addr);
        
        //Check if the response matches one that we are looking for
        packet_type type = packetTracker->match_response(addr, std::string((char *) buffer + 1);
        
        //Parse the response
        EigenResponse *response = packetParser->parse_packet(addr, std::string((char *) buffer + 3));
        if(response->update_module(module)){
            add_module_update(addr, response->type());
        }

        if(response->has_additonal_responses()){
            //Add more packets to the tracker
            for(auto pkt : response->additional_responses())
                packetTracker->add_packet(module->get_address(), pkt, "", type);
        }
        /*
        switch (buffer[3]) { //Terrible code for parsing feedback packets
            case 'S': {
                
                //module->clear_downstream();
                uint8_t count = 0;
                uint8_t topology_updated = 0;
                ptr = buffer + 4;
                ptr = (uint8_t *)strtok ((char *)(buffer+4),",");
                while (ptr != NULL){
                    if(count == 0){
                        //NODE TYPE
                        uint8_t type = strtoul((char *)ptr, NULL, 16);
                        module->update_type(type);
                    } else if(count == 1) {
                        //NODE ORIEANTATION
                        uint8_t orientation = strtoul((char *)ptr, NULL, 16);
                        module->update_orientation(orientation);
                    } else if(count >= 2) {
                        //DOWNSTREAM NEIGHBORS
                        uint8_t down = strtoul((char *)ptr, NULL, 16); //Get the address
                        //ModuleConst mod = get_module(down); //Check that this module exists to protect against garbage data
                        uint8_t ind = count - 2; //index in the downstream list

                        topology_updated |= module->update_downstream(ind, down);
                        
                    }
                    ptr = (uint8_t *)strtok (NULL, ",");
                    count++;
                }
                if(topology_updated)
                    add_module_update(addr, MODULE_DOWNSTREAM);

                break;
                
            }
            case 'N': {
                ptr = buffer + 4;
                uint16_t encoder_status = strtol(((char *)buffer + 4), (char **)&ptr, 16);
                module->set_encoder_status(encoder_status);
                break;
            }
            case 'L': {
                ptr = buffer + 4;
                float encoder_position = strtof((char *)(buffer + 4), (char **)&ptr);
                module->set_position(encoder_position);
                add_module_update(addr, MODULE_POS_UPDATE);
                break;
            }
            case 'V': {
                ptr = buffer + 4;
                float velocity = strtof((char *)(buffer + 4), (char **)&ptr);
                module->set_velocity(velocity);
                add_module_update(addr, MODULE_VEL_UPDATE);
                break;
            }
            case 'I': {
                ptr = buffer + 4;
                float effort = strtof((char *)(buffer + 4), (char **)&ptr);
                module->set_effort(effort);
                add_module_update(addr, MODULE_EFF_UPDATE);
                break;
            }
            case 'U': {
                ptr = buffer + 4;
                uint16_t id = strtoul((char *)(ptr), (char **)&ptr, 16);
                if(id == EIGEN_UTIL_STAT_CODE) {
                    uint8_t status_code = strtoul((char *)(++ptr), (char **)&ptr, 16);
                    if(status_code != module->status_code){
                        firmware_utility(module->get_address(), EIGEN_UTIL_MODULE_STATUS);
                    }
                } else if (id==EIGEN_UTIL_COMMIT_VERSION) { //Firmware Version
                    module->firmware_version = std::string((char *)++ptr);
                } else if (id==EIGEN_UTIL_BUILD_TIME) { //Firmware build time
                    module->firmware_build_time = std::string((char *)++ptr);
                } else if (id==EIGEN_UTIL_BUILD_USER) { //Firmware build user
                    module->firmware_build_name = std::string((char *)++ptr);
                } else if (id==EIGEN_UTIL_GIT_DESCRIBE) { //Firmware tags
                    module->firmware_tag = std::string((char *)++ptr);
                } else if (id==EIGEN_UTIL_MODULE_STATUS) { //Module Status
                    module->t_sync = current_time_ms();
                    module->status_code = strtoul((char *)(++ptr), (char **)&ptr, 16);
                    module->sync_ind = strtoul((char *)(++ptr), (char **)&ptr, 16);
                    module->sync_reg = strtoul((char *)(++ptr), (char **)&ptr, 16);
                    module->LED_code[MAX_LED_CODE_LEN] = strtoul((char *)(++ptr), (char **)&ptr, 16);
                    for(uint8_t i = 0; i < module->LED_code[MAX_LED_CODE_LEN]; i++){
                        module->LED_code[i] = strtoul((char *)(++ptr), (char **)&ptr, 16);
                    }
                    module->module_status = std::string((char *)++ptr);
                } else if (id==EIGEN_UTIL_MODULE_CAPABILITY) { //Module Command Support
                    module->command_support = strtoul((char *)(++ptr), (char **)&ptr, 16);
                } else if (id==EIGEN_UTIL_MODULE_UID) { //UID
                    uint64_t uid = strtoull((char *)(++ptr), (char **)&ptr, 16);
                    module->update_UID(uid);
                } else if (id==EIGEN_UTIL_MODULE_PORTS) { //Module Port List
                    ptr = (uint8_t *)strtok ((char *)(++ptr),",");
                    uint8_t down_count = 0;
                    while (ptr != NULL){
                        if(ptr[0] == 'd'){
                            module->set_downstream_name(down_count, std::string((char *)ptr+2));
                            down_count++;
                        }
                        ptr = (uint8_t *)strtok (NULL, ",");
                    }
                }
                break;
            }

#ifdef EIGEN_BTLDR_SUPPORT
            case '~': {
                ptr = buffer + 4;
                if(ptr[0] == 'r'){
                    ptr++;
                    bootloader_data.push_back(std::string((char *) ptr));
                    //add_module_update(addr, MODULE_BTLDR, std::string((char *) ptr));
                } else if(ptr[0] == 'h'){
                    ptr++;
                    if(bootloader_active == true && bootloader_target_addr == module->get_address()){
                        if(!bootloader_ack) {
                            bootloader_ack = true;
                            acknwoledge_bootload(bootloader_target_addr);

                            int retval = 0;
                            if(bootloader_mode == 1){
                                retval = CyBtldr_Program(bootloader_file.c_str(), NULL, (uint8_t) 3, &comm_struct, &bootloader_update);
                            } else if(bootloader_mode == 2){
                                retval = CyBtldr_Verify(bootloader_file.c_str(), NULL, &comm_struct, &bootloader_update);
                            } else if(bootloader_mode == 3) {
                                retval = CyBtldr_Erase(bootloader_file.c_str(), NULL, &comm_struct, &bootloader_update);
                            }

                            //If successful, retval = 0. Mark as finished.
                            if(!retval){
                                bootloader_finished = true;
                                //uid_write_node_addr(module->get_address(), module->get_UID(), module->get_address());
                            }

                            add_module_update(addr, MODULE_BTLDR_END, bootloader_print_error(retval));
                        } else {
                            acknwoledge_bootload(bootloader_target_addr);
                        }
                    }
                } else if(ptr[0] == 's'){
                    ptr++;
                    (*write_data)((uint8_t *)bootloader_last.c_str(), bootloader_last.length());
                    //add_module_update(addr, MODULE_BTLDR, std::string((char *) ptr));
                }
                break;
            }
#endif
            case '|': {
                ptr = buffer + 4;
                //Handle debug messages
                if(*ptr == ')') { //Parameter write response
                    uint8_t id = strtoul((char *)(++ptr), (char **)&ptr, 16);
                    if(*ptr == ','){
                        ++ptr;
                        if(*ptr == 's'){
                        } else if(*ptr == 'w' || *ptr == 'f') {
                            add_module_update(addr, MODULE_ERROR);
                        }
                    }

                } else if (*ptr == '(') { //Parameter read response
                    uint8_t id = strtoul((char *)(++ptr), (char **)&ptr, 16);
                    if(ptr - (buffer + 5) == 2 && *ptr == ','){
                        //Check for parameter list response
                        if(id == 0){
                            //Push the pointer along, process values
                            uint8_t param_addr = strtol((char *)(++ptr), (char**)&ptr, 16);
                            uint8_t param_aux = strtol((char *)(++ptr), (char**)&ptr, 16);

                            //If we are receiving the number of parameters, fill in that we expect n parameters
                            if(param_addr == 0){
                                for(int i = 1; i < param_aux; i++){
                                    uint8_t s[32];
                                    std::snprintf((char *)s, 32, "|(00,%02X", i);
                                    add_packet(addr, "", std::string((char *)s), EIGEN_PACKET_DEBUG);
                                }

                                module->set_expected_parameters(param_aux);
                                break;
                            }

                            ++ptr;

                            std::string param_name = std::string((char *)ptr);
                            //service_eigencomms should null terminate the string for us

                            module->add_parameter(param_addr, param_aux, param_name);

                            add_module_update(addr, MODULE_PARAM_ADD, param_addr);

                        } else {
                            //TODO: Error checking
                            uint8_t type = module->parameter_type(id);
                            uint64_t value = 0;
                            if(type == _FLOAT || type == _DOUBLE){
                                double val = strtod((char *)(++ptr), NULL);
                                memcpy(&value, &val, sizeof(val));
                            } else {
                                value = strtoull((char *)(++ptr), (char **)&ptr, 16);
                            }
                            //Write value to module
                            module->update_parameter(id, value);

                            //Signal that we need an update
                            add_module_update(addr, MODULE_PARAM_READ, id);
                        }
                    }
                } else { //Regular debug message
                    module->last_debug_msg = std::string((char *)++ptr);
                }
                break;
            }
            default: {
#ifdef EIGEN_BTLDR_SUPPORT
                if(bootloader_active){
                    request_resend_bootload(bootloader_target_addr, "BD-PKT");
                }
#endif
            }
        }
        */
    } else {

#ifdef EIGEN_BTLDR_SUPPORT
        if(bootloader_active){
        //request_resend_bootload(bootloader_target_addr, "BD-PKT");
        }
#endif
    }
}

bool get_poll_enabled(){
    return enabled;
}
void set_poll_enabled(bool state){
    enabled = state;
}

void set_eigen_config(eigen_config config){
    communication_config = config;
}
eigen_config get_eigen_config(){
    return communication_config;
}

int parse_packets(int t_wait_ms, uint8_t *n_chars){
    static uint8_t packet_buffer[IN_BUFFER_SIZE];
    static uint8_t in_buffer[IN_BUFFER_SIZE];
    static uint8_t ptr = 0;

    uint8_t ind = 0;
    uint8_t valid = 1;
    uint8_t packet_count = 0;

    uint16_t count = (*read_data)(in_buffer, IN_BUFFER_SIZE, t_wait_ms);

    if (count > 0) { //Process the incoming data
        while (ind < count) {
            switch (in_buffer[ind]) {
                case 0:
                    if(n_chars != NULL) *n_chars = ptr;
                    return packet_count;
                case '\n':
                case '\r':
                    if (valid) {
                        packet_buffer[ptr] = 0; //Null terminate the string
                        packet_queue.push_back(std::string((char *)packet_buffer));
                        //process_packet(packet_buffer, ptr);
                        packet_count++;
                    }
                    ptr = 0;
                    valid = 1;
                    break;
                default:
                    //Check that this fits in our buffer and that the character is an ASCII character
                    if (ptr < IN_BUFFER_SIZE /*&& in_buffer[ind] < 127*/) {
                        packet_buffer[ptr] = in_buffer[ind];
                        ptr++;
                    } else {
                        valid = 0;
                        break;
                    }
            }
            ind++;
        }
    }

    if(n_chars != NULL) *n_chars = ptr;

    return packet_count;
}

int service_eigen_comms() {
    static uint64_t t_last = current_time_ms();

    uint8_t packet_count = 0;

    if(!enabled) return 0;

    frame_time = current_time_ms() - t_last;
    t_last = current_time_ms();

    packetTracker->handle_timeout_packets();

    //Execute any queued commands
    EigenCommand *cmd = get_command();
    while(cmd != NULL){
        write_packet(cmd->packet().c_str(), cmd->packet().size());
        packetTracker->add_packet(cmd->address_, cmd->expected_response(), cmd->packet(), cmd->type());
        //TODO: add expected response
        delete cmd;
        cmd = get_command();
    }
    /*while(cmd != NULL){
        ModuleShared mod = get_module_shared(cmd->address);
        switch(cmd->command_type){
        case CMD_READ_PARAM:{
            read_parameter(cmd->address, cmd->arg1);
            break;
        } case CMD_WRITE_PARAM:{
            write_parameter(cmd->address, cmd->arg1, cmd->arg2);
            break;
        } case CMD_READ_PARAM_LIST:{
            read_parameter(cmd->address, 0);
            mod->set_param_last_update();
            break;
        } case CMD_SET_MOTOR_ENABLE:{
            motor_enable(cmd->address, cmd->arg1);
            break;
        } case CMD_USER: {
            user_command(cmd->address, cmd->arg2);
            break;
        } case CMD_POS: {
            double pos = set_position(cmd->address, cmd->arg2);
            if(mod != NULL) mod->last_position_cmd = pos;
            break;
        } case CMD_SPEED: {
            double spd = set_speed(cmd->address, cmd->arg2);
            if(mod != NULL) mod->last_velocity_cmd = spd;
            break;
        } case CMD_EFFORT: {
            double eff = set_effort(cmd->address, cmd->arg2);
            if(mod != NULL) mod->last_effort_cmd = eff;
            break;
        } case CMD_RUN: {
            run_command(cmd->address);
            break;
        } case CMD_POLL_TOPOLOGY: {
            poll_topology(cmd->address);
            break;
        } case CMD_UID_WR_ADDR: {
            uid_write_node_addr(cmd->address, cmd->arg1, generate_node_address());
            break;
        }

#ifdef EIGEN_BTLDR_SUPPORT
        case CMD_BTLDR: {
            start_bootload(cmd->address, cmd->arg1, cmd->arg2);
            break;
        }
#endif
        case CMD_ZERO: {
            set_zero(cmd->address);
            break;
        } case CMD_FIRMWARE_UTIL: {
            firmware_utility(cmd->address, cmd->arg1);
        } default:
            break;
        }

        delete cmd;
        cmd = get_command();
    }*/


    /* POLLING */
    if(current_time_ms() - t_last_update_poll > UPDATE_TOPOLOGY_PERIOD){
        poll_topology(0xFF);
        t_last_update_poll = current_time_ms();
    }

    if(current_time_ms() - t_last_status_poll > UPDATE_STATUS_PERIOD){
        //Poll module statuses
        firmware_utility(0xFF, EIGEN_UTIL_STAT_CODE);
        t_last_status_poll = current_time_ms();
    }

    //If we are due to poll, communicate with the module
    if (current_time_ms() - t_last_poll > UPDATE_PERIOD) {
        if(communication_config.poll_encoder_status == EIGEN_ENABLED)
            poll_status(0xFF, EIGEN_POLL_ENC_STATUS);
        if(communication_config.poll_module_position == EIGEN_ENABLED)
            poll_status(0xFF, EIGEN_POLL_LOCATION);
        if(communication_config.poll_module_velocity == EIGEN_ENABLED)
            poll_status(0xFF, EIGEN_POLL_VELOCITY);
        if(communication_config.poll_module_effort == EIGEN_ENABLED)
            poll_status(0xFF, EIGEN_POLL_EFFORT);

        t_last_poll = current_time_ms();
    }

    packet_count = parse_packets(0, NULL);
    while(packet_queue.size() > 0){
        std::string packet = packet_queue.front();
        packet_queue.pop_front();
        process_packet((uint8_t *)packet.c_str(), packet.size());
    }

    //Check that the module's parameters are updated properly
    for(uint8_t ind = 0; ind < num_modules(); ind++){
        ModuleShared mod = get_module_shared(ind);
        if(mod->parameters_left() > 0 && mod->d_t_param_last_update() > PACKET_TIMEOUT){
            add_command(mod->get_address(), CMD_READ_PARAM_LIST, 0);
        }
    }

    //Handle the successful packets
    packetTracker->handle_successful_packets();

    //Mark the stale modules as such
    clean_module_list();

    return packet_count;
}

void add_command(eigen_command *command){
    std::lock_guard<std::mutex> lock(cmd_mutex);
    cmd_list.push_back(command);
}

void add_command(uint8_t addr, command_enum cmd_type, uint64_t arg1){
    add_command(addr, cmd_type, arg1, "");
}

void add_command(uint8_t addr, command_enum cmd_type, std::string arg2){
    add_command(addr, cmd_type, 0, arg2);
}

void add_command(uint8_t addr, command_enum cmd_type, uint64_t arg1, std::string arg2){
    eigen_command *command = new eigen_command;
    command->command_type = cmd_type;
    command->address = addr;
    command->arg1 = arg1;
    command->arg2 = arg2;

    add_command(command);
}

void clear_commands(){
    std::lock_guard<std::mutex> lock(cmd_mutex);
    cmd_list.clear();
}

EigenCommand *get_command(){
    EigenCommand *retval;
    std::lock_guard<std::mutex> lock(cmd_mutex);

    if(cmd_list.size() > 0){
        retval = cmd_list.front();
        cmd_list.pop_front();
    } else {
        return NULL;
    }

    return retval;
}

bool mod_cmp(Module *m1, Module *m2){
    return (m1->get_address()) < (m2->get_address());
}

bool mod_cmp_low_bnd(Module *m1, uint8_t addr){
    return (m1->get_address()) < addr;
}

ModuleShared get_module_shared(uint8_t address){
    std::lock_guard<std::mutex> lock(module_list_mutex);

    ModuleShared mod_result = NULL;
    if(module_list.size() > 0){
        for(auto mod : module_list){
            if(mod->get_address() == address){
                mod_result = mod;
                break;
            }
        }
    }

    if(mod_result != NULL && mod_result->get_address() != address) mod_result = NULL;

    return mod_result;
}

ModuleShared add_module(uint8_t address){
    std::lock_guard<std::mutex> lock(module_list_mutex);

    //qDebug() << address;

    ModuleShared mod_result = NULL;
    if(module_list.size() > 0){
        for(auto mod : module_list){
            if(mod->get_address() == address){
                mod_result = mod;
                break;
            }
        }
    }
    if(mod_result == NULL || mod_result->get_address() != address){
        mod_result = std::make_shared<Module>(address);
        module_list.push_back(mod_result);
        //std::sort(module_list.begin(), module_list.end(), mod_cmp);

        //Log that we updated the list
        list_update = true;
        add_module_update(address, MODULE_ADDED);

        //Ask for important info about the module
        firmware_utility(address, EIGEN_UTIL_COMMIT_VERSION);
        firmware_utility(address, EIGEN_UTIL_BUILD_TIME);
        firmware_utility(address, EIGEN_UTIL_BUILD_USER);
        firmware_utility(address, EIGEN_UTIL_GIT_DESCRIBE);
        firmware_utility(address, EIGEN_UTIL_MODULE_CAPABILITY);
        firmware_utility(address, EIGEN_UTIL_MODULE_PORTS);
        firmware_utility(address, EIGEN_UTIL_MODULE_UID);
        firmware_utility(address, EIGEN_UTIL_MODULE_STATUS);
    } else {
        add_module_update(address, MODULE_TOUCHED);
    }
    mod_result->t_last_update = current_time_ms();
    mod_result->stale = false;

    return mod_result;
    /*
    //TODO: FIX THIS

    //Avoid overwriting if the module is already in the list
    auto low = std::lower_bound(module_list.begin(), module_list.end(), address, mod_cmp_low_bnd);
    if(low == module_list.end()){ //If the module does not exist
        module_list.push_back(new Module(address));
        std::sort(module_list.begin(), module_list.end(), mod_cmp);

        //Log that we updated the list
        list_update = true;
        add_module_update(address, MODULE_ADDED);

        firmware_utility(address, 0x02);
        firmware_utility(address, 0x04);
        firmware_utility(address, 0x08);
        firmware_utility(address, 0x10);
        firmware_utility(address, 0x40);

        //Find the module again in the sorted list and return
        low = std::lower_bound(module_list.begin(), module_list.end(), address, mod_cmp_low_bnd);
    } else {
        //qDebug() << (*low)->get_address();
        //Mark that we have updated this particular module
        add_module_update(address, MODULE_TOUCHED);
    }

    (*low)->t_last_update = current_time_ms();
    (*low)->stale = false;

    return *low;*/
}

ModuleConst get_module(uint8_t address){
    std::lock_guard<std::mutex> lock(module_list_mutex);

    /*
    auto low = std::lower_bound(module_list.begin(), module_list.end(), address, mod_cmp_low_bnd);
    if(low == module_list.end()){ //If the module does not exist
        return *null_mod;
    }*/

    ModuleShared mod_result = NULL;
    if(module_list.size() > 0){
        for(auto mod : module_list){
            if(mod->get_address() == address){
                mod_result = mod;
                break;
            }
        }
    }
    if(mod_result == NULL || mod_result->get_address() != address){
        return NULL;
    } else {
        return std::const_pointer_cast<const Module>(mod_result);
    }

}

ModuleConst get_module_by_index(uint8_t index){
    std::lock_guard<std::mutex> lock(module_list_mutex);

    if(index >= module_list.size()) return NULL;

    ModuleShared mod = module_list[index];
    return std::const_pointer_cast<const Module>(mod);
}

uint8_t num_modules(){
    std::lock_guard<std::mutex> lock(module_list_mutex);

    return module_list.size();
}

std::vector<ModuleShared> *get_module_list(){
    return &module_list;
}

/* Clear on read */
bool list_updated(){
    if(list_update){
        list_update = false;
        return true;
    } else {
        return false;
    }
}
