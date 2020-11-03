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

static std::deque<std::string> bootloader_data;
static std::deque<std::string> packet_queue;

static bool list_update;
static bool enabled;
static eigen_config communication_config;

#ifdef EIGEN_BTLDR_SUPPORT
static CyBtldr_CommunicationsData comm_struct;
static bool bootloader_active;
static bool bootloader_ack;
static uint8_t bootloader_target_addr;
static uint8_t bootloader_mode;
static std::string bootloader_file;
static bool bootloader_finished;
static std::string bootloader_last;
static uint8_t bootloader_seq_num;
#endif

/* Private forward declarations */
void clear_module_list();
EigenCommand *get_command();
void add_module_update(uint8_t addr, update_enum type);
void add_module_update(uint8_t addr, update_enum type, uint8_t arg);
uint8_t generate_node_address();

#ifdef EIGEN_BTLDR_SUPPORT
int bootloader_open();
int bootloader_close();
int bootloader_read_data(uint8_t* buf, int len);
int bootloader_write_data(uint8_t* buf, int len);
void bootloader_update(uint8_t col, uint16_t row);
void bootloader_init();
std::string bootloader_print_error(int retval);
#endif

//Bus functions
double set_position(uint8_t address, std::string val);
void run_command(uint8_t address);
double set_speed(uint8_t address, std::string val);
double set_effort(uint8_t address, std::string val);
void poll_status(uint8_t address, uint8_t value);
void poll_topology(uint8_t address);
void read_EEPROM(uint8_t module, uint16_t address, uint8_t size);
void write_EEPROM(uint8_t module, uint16_t address, uint8_t value);
void firmware_utility(uint8_t address, uint16_t action);
void motor_enable(uint8_t module, uint8_t value);
void user_command(uint8_t module, std::string command);
void read_parameter(uint8_t module, uint8_t id);
void write_parameter(uint8_t module, uint8_t id, std::string param_value);
void uid_write_node_addr(uint8_t address, uint64_t uid, uint8_t node_addr);
void set_zero(uint8_t address);
#ifdef EIGEN_BTLDR_SUPPORT
void start_bootload(uint8_t address, uint8_t mode, std::string file);
void acknwoledge_bootload(uint8_t address);
void request_resend_bootload(uint8_t address, std::string msg);
#endif

//Module list interface functions
ModuleShared add_module(uint8_t address);
ModuleShared get_module_shared(uint8_t address);

/* Table for CRC-8-CCITT from https://www.3dbrew.org/wiki/CRC-8-CCITT */
static const uint8_t CRC_8_TABLE[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
    0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
    0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
    0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
    0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
    0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
    0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
    0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
    0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
    0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
    0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
    0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

uint8_t crc_8_ccitt(uint8_t *data, uint16_t len){
    uint8_t crc = 0xFF; //Seed of 0xFF

    for(uint16_t ind = 0; ind < len; ind++){
        uint8_t temp = crc ^ data[ind];
        crc = CRC_8_TABLE[temp];
    }

    return crc; //Final of 0x00
}

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
        packetTracker->match_response(addr, std::string((char *) buffer + 1);
        
        //Parse the response
        EigenResponse *response = packetParser->parse_packet(std::string((char *) buffer + 3));
        if(response->update_module(module)){
            add_module_update(addr, response->type());
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
/*
double set_position(uint8_t address, std::string val) {
    uint8_t s[32];
    float f = atof(val.c_str());
    int count = std::snprintf((char *)s, 32, "%02xP%08.4f", address, f);
    write_packet(s, count);

    //Not expecting a response
    add_packet(address, std::string((char *) s), "");

    return f;
}*/

void run_command(uint8_t address){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 32, "%02xR", address);
    write_packet(s, count);

    //Not expecting a response
    add_packet(address, std::string((char *) s), "");
}
/*
double set_speed(uint8_t address, std::string val) {
    uint8_t s[32];
    float f = atof(val.c_str());
    int count = std::snprintf((char *)s, 32, "%02xS%08.4f", address, f);
    write_packet(s, count);

    //Not expecting a response
    add_packet(address, std::string((char *) s), "");

    return f;
}

double set_effort(uint8_t address, std::string val) {
    uint8_t s[32];
    float f = atof(val.c_str());
    int count = std::snprintf((char *)s, 32, "%02xT%08.4f", address, f);
    write_packet(s, count);

    //Not expecting a response
    add_packet(address, std::string((char *) s), "");

    return f;
} */

void poll_status(uint8_t address, uint8_t value){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 32, "%02xQ%02x", address, value);
    write_packet(s, count);

    switch(value){
        case EIGEN_POLL_LOCATION: {
            //Expecting a response of type L
            add_packet(address, std::string((char *) s), "L", EIGEN_PACKET_POLL);
            break;
        } case EIGEN_POLL_EFFORT: {
            //Expecting a response of type I
            add_packet(address, std::string((char *) s), "I", EIGEN_PACKET_POLL);
            break;
        } case EIGEN_POLL_ENC_STATUS: {
            //Expecting a response of type N
            add_packet(address, std::string((char *) s), "N", EIGEN_PACKET_POLL);
            break;
        } default: {
            //Unsupported, not expecting a response
            add_packet(address, std::string((char *) s), "");
            break;
        }
    }

}
/*
void poll_topology(uint8_t address){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 32, "%02xO", address);
    write_packet(s, count);

    //Expecting a response of type H
    add_packet(address, std::string((char *) s), "S", EIGEN_PACKET_TOPO);
}*/

void read_EEPROM(uint8_t module, uint16_t address, uint8_t size){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 32, "%02x>%04x,%02x,7491821", module, address, size);
    write_packet(s, count);

    //Expecting a response of type ^
    add_packet(address, std::string((char *) s), "^", EIGEN_PACKET_DEBUG);
}

void write_EEPROM(uint8_t module, uint16_t address, uint8_t value){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 32, "%02x>%04x,%02x,7491821", module, address, value);
    write_packet(s, count);

    //Expecting a response of type |EEPROM
    add_packet(address, std::string((char *) s), "|EEPROM", EIGEN_PACKET_DEBUG);
}

void read_parameter(uint8_t module, uint8_t id){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 32, "%02x(%02x", module, id);
    write_packet(s, count);

    //Expecting a response of type |(
    std::string cmd = std::string((char *) s);
    std::snprintf((char *)s, 32, "|(%02X", id);
    add_packet(module, cmd, std::string((char *)s), EIGEN_PACKET_DEBUG);
}

void write_parameter(uint8_t module, uint8_t id, std::string param_value){
    uint8_t s[32];
    int count = 0;

    ModuleConst mod = get_module(module);
    uint8_t type = mod->parameter_type(id);

    switch(type){
    case _UINT8: {
        uint8_t val = atol(param_value.c_str());
        count = snprintf((char *)s, 32, "%02X)%02X,%02X,8675309", module, id, val);
        break;
    } case _UINT16: {
        uint16_t val = atol(param_value.c_str());
        count = snprintf((char *)s, 32, "%02X)%02X,%04X,8675309", module, id, val);
        break;
    } case _UINT32: {
        uint32_t val = atol(param_value.c_str());
        count = snprintf((char *)s, 32, "%02X)%02X,%08X,8675309", module, id, val);
        break;
    } case _UINT64: {
        uint64_t val = atol(param_value.c_str());
        count = snprintf((char *)s, 32, "%02X)%02X,%016llX,8675309", module, id, val);
        break;
    } case _FLOAT: {
        float val = atof(param_value.c_str());
        count = snprintf((char *)s, 32, "%02X)%02X,%08.4f,8675309", module, id, val);
        break;
    } case _DOUBLE: {
        double val = atof(param_value.c_str());
        count = snprintf((char *)s, 32, "%02X)%02X,%08.4f,8675309", module, id, val);
        break;
    } default: {
        snprintf((char *)s, 32, "ERR\n");
        break;
    }
    }

    write_packet(s, count);

    //If we did not send a packet, we do not want to wait for a response
    if(count == 0) return;

    //Expecting a response of type |)
    std::string cmd = std::string((char *) s);
    std::snprintf((char *)s, 32, "|)%02X", id);
    add_packet(module, cmd, std::string((char *)s), EIGEN_PACKET_DEBUG);
}

void uid_write_node_addr(uint8_t addr, uint64_t uid, uint8_t node_addr){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 32, "u%016llx)01,%02x,8675309", uid, node_addr);
    write_packet(s, count);

    //Expecting a response of type |)
    std::string cmd = std::string((char *) s);
    add_packet(addr, cmd, "|)01", EIGEN_PACKET_DEBUG);
}

void firmware_utility(uint8_t address, uint16_t action){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 32, "%02xU%02X", address, action);
    write_packet(s, count);

    //Expecting a response of type U
    std::string cmd = std::string((char *) s);
    std::snprintf((char *)s, 32, "U%02X", action);
    add_packet(address, cmd, std::string((char *)s), EIGEN_PACKET_POLL);
}

void motor_enable(uint8_t module, uint8_t value){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 100, "%02xM%02x", module, value);
    write_packet(s, count);

    //Expecting no response
    std::string cmd = std::string((char *) s);
    add_packet(module, cmd, "");
}

void user_command(uint8_t module, std::string command){
    write_packet((uint8_t *)command.c_str(), command.size());

    //Not sure what the response will be, so we put no response
    add_packet(module, command, "");
}


void set_zero(uint8_t address){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 100, "%02xZ", address);
    write_packet(s, count);

    //Expecting no response
    std::string cmd = std::string((char *) s);
    add_packet(address, cmd, "");
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
