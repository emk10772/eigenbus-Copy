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
#define UPDATE_PERIOD           (100)
#define UPDATE_TOPOLOGY_PERIOD  (1000)
#define UPDATE_STATUS_PERIOD    (250)
#define PACKET_TIMEOUT          (600)

#define ADDR_PARAM              (1)
#define TYPE_PARAM              (2)

using ModuleShared = std::shared_ptr<Module>;

//Private Variables
static uint16_t (*read_data)(uint8_t *buf, uint16_t max_len, int t_wait_ms);
static void (*write_data)(uint8_t *buf, uint16_t len);

static uint64_t t_last_poll = 0;
static uint64_t t_last_update_poll = 0;
static uint64_t t_last_status_poll = 0;
static uint64_t t_init = 0;

static uint64_t sent_packets = 0;
static uint64_t dropped_packets = 0;
static uint64_t successful_packets = 0;
static uint64_t unrequested_packets = 0;
static uint64_t retried_packets = 0;
static uint64_t frame_time = 0;
static std::string last_dropped = "";

//Module data structure
//static std::map<uint8_t, Module *> module_map;
static std::vector<std::shared_ptr<Module>> module_list;
std::mutex module_list_mutex;

//Interface deques and mutexes for thread safety
static std::deque<module_update *> update_list;
static std::mutex update_mutex;
static std::deque<eigen_command *> cmd_list;
static std::mutex cmd_mutex;
static std::deque<raw_packet *> raw_packet_list;
static std::mutex raw_packet_mutex;

//Internal deque, does not need to be thread safe
static std::deque<EigenPacketFilter> packet_filter_list;
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
eigen_command *get_command();
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

#ifdef MODULE_TEST
    Module *mod1 = add_module(0x01);
    Module *mod2 = add_module(0x02);
    Module *mod3 = add_module(0x03);
    Module *mod4 = add_module(0x04);
    Module *mod5 = add_module(0x05);
    Module *mod6 = add_module(0x06);

    mod1->add_downstream(0x02);
    add_module_update(0x01, MODULE_DOWNSTREAM);
    mod1->add_downstream(0x03);
    add_module_update(0x01, MODULE_DOWNSTREAM);
    mod2->add_downstream(0x04);
    add_module_update(0x02, MODULE_DOWNSTREAM);
    mod4->add_downstream(0x05);
    add_module_update(0x04, MODULE_DOWNSTREAM);
    mod4->add_downstream(0x06);
    add_module_update(0x04, MODULE_DOWNSTREAM);

#endif
}

void clean_eigen_comms() {
    //TODO: Free each module, clean up the map

    clear_module_list();
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

raw_packet *get_raw_packet(){
    raw_packet *retval;
    std::lock_guard<std::mutex> lock(raw_packet_mutex);

    if(raw_packet_list.size() > 0){
        retval = raw_packet_list.front();
        raw_packet_list.pop_front();
    } else {
        return NULL;
    }

    return retval;
}

void add_raw_packet(std::string pkt_string, packet_type type, uint8_t dir){
    if(communication_config.raw_packet_en == EIGEN_DISABLED) return;

    std::lock_guard<std::mutex> lock(raw_packet_mutex);

    //Add to raw packet list
    raw_packet *pkt = new raw_packet;
    pkt->packet = pkt_string;
    pkt->type = type;
    pkt->dir = dir;
    raw_packet_list.push_back(pkt);
}

void add_packet(uint8_t address, std::string pkt_string, std::string filter, packet_type packet = EIGEN_PACKET_DEFAULT){
    //Add the filter to the filter list and the raw packet to the raw packet list if it is not empty
    packet_filter_list.push_back(EigenPacketFilter(address, filter, packet, pkt_string));
    if(pkt_string != "")
        add_raw_packet(pkt_string, packet, EIGEN_PACKET_SEND);

    //Track that we sent a packet
    sent_packets++;
}

void handle_timeout_packets(){
    //Newest packets are put into this queue from the back. Will be sorted in order of time because of this
    //Clear the timed out packets from the front of the queue
    auto it = packet_filter_list.begin();
    while(it != packet_filter_list.end() && it->packet_timeout()){
        if(it->is_broadcast() && it->num_responses() > 0){
            //Bar for success for a broadcast packet is pretty low. Just want to get at least one response
            successful_packets++;
        } else if (it->is_broadcast() || it->num_retries() >= communication_config.max_retries){
            dropped_packets++;
            last_dropped = it->packet_string();
        } else {
            retried_packets++;

            //Reset the timer, and increase the number of tries. Then resend the packet
            EigenPacketFilter packet = *it;
            packet.increment_retry_count();
            packet.reset_timeout();
            packet_filter_list.push_back(packet);

            write_packet((uint8_t *)packet.packet_string().c_str(), packet.packet_string().length());
        }

        packet_filter_list.pop_front();
        it = packet_filter_list.begin();
    }
}

void handle_successful_packets(){
    auto it = packet_filter_list.begin();
    //Check for successful packets and remove them from the list
    while(it != packet_filter_list.end()){
        if(it->expects_response()){
            if(!it->is_broadcast() && it->num_responses() > 0) {
                successful_packets++;
                it = packet_filter_list.erase(it);
            } else {
                it++;
            }
        } else {
            it = packet_filter_list.erase(it);
            successful_packets++;
        }
    }
}

bool match_response(uint8_t address, std::string packet){
    //Search our filter list for a matching packet
    auto it = packet_filter_list.begin();
    while(it != packet_filter_list.end() && !it->matches_filter(address, packet)){
        it++;
    }

    //If we found a matching filter, add it to that filter
    if(it != packet_filter_list.end() && it->matches_filter(address, packet)){
        it->add_response(address, packet);
        add_raw_packet(packet, it->get_type(), EIGEN_PACKET_RECV);
        return true;
    } else {
        add_raw_packet(packet, EIGEN_PACKET_DEFAULT, EIGEN_PACKET_RECV);
        unrequested_packets++;

        //If we get an unrequested packet for a valid address there are a few possibilities:
        //1. Duplicate addresses
        //2. Garbled packet
        //To be sure that we have no duplicate addresses, poll the UIDs for this particular address

#ifdef EIGEN_BTLDR_SUPPORT
        if(!bootloader_active)
            firmware_utility(0xFF, EIGEN_UTIL_MODULE_UID);
#else
        firmware_utility(0xFF, EIGEN_UTIL_MODULE_UID);
#endif
    }

    return false;
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
        match_response(addr, std::string((char *)(buffer + 1)));

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
                        /*if(down != 0xFF && mod != NULL){ //If it is a valid address and the module exists
                            module->update_downstream(ind, down);
                        } else {
                            module->update_downstream(ind, down);
                        }*/
                    }
                    ptr = (uint8_t *)strtok (NULL, ",");
                    count++;
                }
                /* If we got updated topology info */
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

    //Check if any packets have timed out
    handle_timeout_packets();

    //Execute any queued commands
    eigen_command *cmd = get_command();
    while(cmd != NULL){
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
        } default:
            break;
        }

        delete cmd;
        cmd = get_command();
    }


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
    handle_successful_packets();

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

eigen_command *get_command(){
    eigen_command *retval;
    std::lock_guard<std::mutex> lock(cmd_mutex);

    if(cmd_list.size() > 0){
        retval = cmd_list.front();
        cmd_list.pop_front();
    } else {
        return NULL;
    }

    return retval;
}

double set_position(uint8_t address, std::string val) {
    uint8_t s[32];
    float f = atof(val.c_str());
    int count = std::snprintf((char *)s, 32, "%02xP%08.4f", address, f);
    write_packet(s, count);

    //Not expecting a response
    add_packet(address, std::string((char *) s), "");

    return f;
}

void run_command(uint8_t address){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 32, "%02xR", address);
    write_packet(s, count);

    //Not expecting a response
    add_packet(address, std::string((char *) s), "");
}

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
}

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

void poll_topology(uint8_t address){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 32, "%02xO", address);
    write_packet(s, count);

    //Expecting a response of type H
    add_packet(address, std::string((char *) s), "S", EIGEN_PACKET_TOPO);
}

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

#ifdef EIGEN_BTLDR_SUPPORT
void start_bootload(uint8_t address, uint8_t mode, std::string file){
    firmware_utility(address, EIGEN_UTIL_MODULE_STATUS);

    uint8_t s[32];
    int count = std::snprintf((char *)s, 100, "%02x~b", address);
    write_packet(s, count);
    bootloader_ack = false;
    bootloader_target_addr = address;
    bootloader_active = true;
    bootloader_mode = mode;
    bootloader_file = file;

    //Expecting no response
    std::string cmd = std::string((char *) s);
    add_packet(address, cmd, "");
}

void acknwoledge_bootload(uint8_t address){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 100, "%02x~a", address);
    write_packet(s, count);

    //Expecting no response
    std::string cmd = std::string((char *) s);
    add_packet(address, cmd, "");
}

void request_resend_bootload(uint8_t address, std::string msg){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 100, "%02x~s,%s", address, msg.c_str());
    write_packet(s, count);

    //Expecting no response
    std::string cmd = std::string((char *) s);
    add_packet(address, cmd, "");
}
#endif

void set_zero(uint8_t address){
    uint8_t s[32];
    int count = std::snprintf((char *)s, 100, "%02xZ", address);
    write_packet(s, count);

    //Expecting no response
    std::string cmd = std::string((char *) s);
    add_packet(address, cmd, "");
}

/* Module Class Definitions */

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

/* Bootloader Interface */
#ifdef EIGEN_BTLDR_SUPPORT
#define MAX_PACKET_CHARS    60
#define OUT_BUF_SIZE        128

int bootloader_open(void){
    bootloader_finished = false;
    bootloader_seq_num = 0;

    if(bootloader_ack) {
        return CYRET_SUCCESS;
    }

    return CYRET_ABORT;
}

int bootloader_close(void){
    bootloader_active = false;
    bootloader_target_addr = 0xFF;
    bootloader_ack = false;

    return CYRET_SUCCESS;
}

static unsigned short crc_table [256] = {

0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5,
0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b,
0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210,
0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c,
0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401,
0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b,
0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738,
0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5,
0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969,
0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc,
0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03,
0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6,
0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb,
0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1,
0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c,
0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2,
0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb,
0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447,
0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2,
0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827,
0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c,
0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0,
0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d,
0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07,
0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba,
0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

unsigned short CRCCCITT(unsigned char *data, size_t length, unsigned short seed, unsigned short final)
{

   size_t count;
   unsigned int crc = seed;
   unsigned int temp;

   for (count = 0; count < length; ++count)
   {
     temp = (*data++ ^ (crc >> 8)) & 0xff;
     crc = crc_table[temp] ^ (crc << 8);
   }

   return (unsigned short)(crc ^ final);

}

#define PACKET_SIZE_LEN     (2) //"%02X"
#define PACKET_CRC_LEN      (4) //"%04X"
#define PACKET_SEQ_LEN      (2) //"%02X"
#define PACKET_HEADER_LEN   (PACKET_SIZE_LEN + 1 + PACKET_CRC_LEN + 1 + PACKET_SEQ_LEN + 1) //"%02X,%04X,%02X."
#define PROCESS_BUFFER_SIZE (256)

bool bootloader_parse_packet(std::string data, uint8_t *buf, uint8_t len){
    uint8_t packet_len = data.size() - PACKET_HEADER_LEN;
    if(data.size() < PACKET_HEADER_LEN || packet_len % 2){
        request_resend_bootload(bootloader_target_addr, "LEN");
        return false;
    }

    uint8_t ind = 0;
    unsigned int c = 0;
    char aux_buffer[PROCESS_BUFFER_SIZE];
    uint8_t valid = 1;

    // Process the header
    char *ptr = (char *)data.c_str();
    uint8_t num_chars = strtol(ptr, &ptr, 16);
    if(*ptr != ',') valid = 0;
    ++ptr;
    uint16_t crc_packet = strtol(ptr, &ptr, 16);
    if(*ptr != ',') valid = 0;
    ++ptr;
    uint8_t sequence_num = strtol(ptr, &ptr, 16);
    if(*ptr != ',') valid = 0;
    ++ptr;

    /* Move chars into the buffer until we run out of space or data */
    while(valid && 2*ind < packet_len){
        if(!isxdigit((uint8_t)(data.c_str()[2*ind + PACKET_HEADER_LEN])) ||
                !isxdigit((uint8_t)(data.c_str()[2*ind + PACKET_HEADER_LEN + 1]))){
            valid = 0;
            break;
        }

        int retval = sscanf((char *)data.c_str() + 2*ind + PACKET_HEADER_LEN, "%02x", &c);

        if(retval != 1){
            valid = 0;
            break;
        }

        aux_buffer[ind] = (uint8_t) c;
        ind += 1;
    }

    uint16_t crc_calc = CRCCCITT((uint8_t *)aux_buffer, ind, 0xFFFF, 0);

    if(!valid){
        request_resend_bootload(bootloader_target_addr, "INVLD");
        return false;
    } else if((2*ind) != packet_len){
        request_resend_bootload(bootloader_target_addr, "LEN");
        return false;
    } else if(ind != num_chars) {
        request_resend_bootload(bootloader_target_addr, "CHAR");
        return false;
    } else if (crc_calc != crc_packet) {
        request_resend_bootload(bootloader_target_addr, "CRC");
        return false;
    } else if (sequence_num != bootloader_seq_num){
        (*write_data)((uint8_t *)bootloader_last.c_str(), bootloader_last.length());
        //request_resend_bootload(bootloader_target_addr, "SEQ");
        return false;
    } else {
        //Put all of the data in the buffer
        for(uint8_t i = 0; i < ind; i++){
            buf[i] = aux_buffer[i];
        }
        return true;
    }
}

#include <QDebug>

int bootloader_read_data(uint8_t* buf, int len){
    uint64_t t_start = current_time_ms();
    uint64_t t_last = t_start;
    uint8_t n_chars = 0;
    uint8_t first_request_n = 0;

    while(/*bootloader_data.size() == 0 &&*/ current_time_ms() - t_start < 500000){
        parse_packets(250, &n_chars);
        //(*write_data)((uint8_t *)"05U20\n", 6);
        //qDebug() << n_chars << packet_queue.size() << bootloader_data.size() << '\n';
        if((packet_queue.size() == 0 && current_time_ms() - t_last > 250)
                || (n_chars > 0 && current_time_ms() - t_last > 250)){
            //char out_buf[OUT_BUF_SIZE];
            //uint8_t ind = snprintf(out_buf, OUT_BUF_SIZE, "%02X~s\n", bootloader_target_addr);
            //(*write_data)((uint8_t *)out_buf, ind);
            if(!first_request_n){
                first_request_n = 1;
                (*write_data)((uint8_t *)"\n", 1);
            } else if(current_time_ms() - t_last > 750) {
                request_resend_bootload(bootloader_target_addr, "TIME");
            }
        }

        while(packet_queue.size() > 0){
            std::string packet = packet_queue.front();
            packet_queue.pop_front();
            process_packet((uint8_t *)packet.c_str(), packet.size());

            t_last = current_time_ms();
        }

        if(bootloader_data.size() > 0){
            std::string data = bootloader_data.front();
            bootloader_data.pop_front();

            bool success = bootloader_parse_packet(data, buf, len);
            if(success) return CYRET_SUCCESS;
        }
    }

    return CYRET_ERR_UNK;
    /*if(bootloader_data.size() == 0) return CYRET_ERR_UNK;

    std::string data = bootloader_data.front();
    bootloader_data.pop_front();

    if(data.size() > 0 && data.size() % 2 == 0){

        uint8_t ind = 0;

        unsigned int c = 0;


        while(2*ind < data.size() && ind < len){
            sscanf((char *)data.c_str() + 2*ind, "%02x", &c); //TODO: Error checking

            buf[ind] = (char) c;
            ind ++;
        }

        return CYRET_SUCCESS;
    } else {
        return CYRET_ERR_UNK;
    }*/
}

int bootloader_write_data(uint8_t* buf, int len){
    char out_buf[OUT_BUF_SIZE];

    uint16_t crc = CRCCCITT(buf, len, 0xFFFF, 0);

    uint16_t packet_ind = 0;
    while(packet_ind < len){
        //Print the header
        uint8_t ct = 0;
        uint8_t ind = snprintf(out_buf, OUT_BUF_SIZE, "%02X~d%02X,%04X,%02X,", bootloader_target_addr, len, crc, bootloader_seq_num);

        //Print the data characters
        while(ind < OUT_BUF_SIZE && ct < MAX_PACKET_CHARS && packet_ind < len){
            ind += snprintf(out_buf + ind, OUT_BUF_SIZE - ind, "%02X", buf[packet_ind]);
            ct++;
            packet_ind++;
        }

        uint8_t crc_2 = crc_8_ccitt((uint8_t *)out_buf, ind);

        //Print the footer
        ind += snprintf(out_buf + ind, OUT_BUF_SIZE - ind, ":%02X\n", crc_2);

        bootloader_last = std::string(out_buf);
        (*write_data)((uint8_t *)out_buf, ind);
        //CyDelay(1);
    }

    //Increase the sequence counter for each packet sent
    bootloader_seq_num++;
    add_packet(bootloader_target_addr, out_buf, "~r");
    return CYRET_SUCCESS;
}

void bootloader_update(uint8_t col, uint16_t row){
    add_module_update(bootloader_target_addr, MODULE_BTLDR_PROGRESS, row);
}

void bootloader_init(){
    comm_struct.MaxTransferSize = 52;
    comm_struct.OpenConnection = &bootloader_open;
    comm_struct.CloseConnection = &bootloader_close;
    comm_struct.ReadData = &bootloader_read_data;
    comm_struct.WriteData = &bootloader_write_data;
}

bool is_bootloader_active(){
    return bootloader_active;
}

bool is_bootloader_finished(){
    if(bootloader_finished){
        bootloader_finished = false;
        return true;
    }
    return false;
}

std::string bootloader_print_error(int retval){
    std::string printed;
    if(retval & CYRET_ERR_COMM_MASK){
        printed = "Communications Error: ";
        int masked = retval & 0xFF;

        if(masked == CYRET_SUCCESS){
            printed.append("Completed Successfully");
        } else if(masked == CYRET_ERR_FILE){
            printed.append("File is not accessible");
        } else if(masked == CYRET_ERR_EOF){
            printed.append("Reached the end of the file");
        } else if(masked == CYRET_ERR_LENGTH){
            printed.append("The amount of data available is outside the expected range");
        } else if(masked == CYRET_ERR_DATA){
            printed.append("The data is not of the proper form");
        } else if(masked == CYRET_ERR_CMD){
            printed.append("The command is not recognized");
        } else if(masked == CYRET_ERR_DEVICE){
            printed.append("The expected device does not match the detected device");
        } else if(masked == CYRET_ERR_VERSION){
            printed.append("The bootloader version detected is not supported");
        } else if(masked == CYRET_ERR_CHECKSUM){
            printed.append("The checksum does not match the expected value");
        } else if(masked == CYRET_ERR_ARRAY){
            printed.append("The flash array is not valid");
        } else if(masked == CYRET_ERR_ROW){
            printed.append("The flash row is not valid");
        } else if(masked == CYRET_ERR_BTLDR){
            printed.append("The bootloader is not ready to process data");
        } else if(masked == CYRET_ERR_ACTIVE){
            printed.append("The application is currently marked as active");
        } else if(masked == CYRET_ERR_UNK){
            printed.append("An unknown error occurred");
        } else if(masked == CYRET_ABORT){
            printed.append("The operation was aborted");
        }
    } else if(retval & CYRET_ERR_BTLDR_MASK) {
        printed = "Bootloader Error: ";
        int masked = retval & 0xFF;

        if(masked == CYBTLDR_STAT_SUCCESS){
            printed.append("Completed Successfully");
        } else if(masked == CYBTLDR_STAT_ERR_KEY){
            printed.append("The provided key does not match the expected value");
        } else if(masked == CYBTLDR_STAT_ERR_VERIFY){
            printed.append("The verification of flash failed");
        } else if(masked == CYBTLDR_STAT_ERR_LENGTH){
            printed.append("The amount of data available is outside the expected range");
        } else if(masked == CYBTLDR_STAT_ERR_DATA){
            printed.append("The data is not of the proper form");
        } else if(masked == CYBTLDR_STAT_ERR_CMD){
            printed.append("The command is not recognized");
        } else if(masked == CYBTLDR_STAT_ERR_DEVICE){
            printed.append("The expected device does not match the detected device");
        } else if(masked == CYBTLDR_STAT_ERR_VERSION){
            printed.append("The bootloader version detected is not supported");
        } else if(masked == CYBTLDR_STAT_ERR_CHECKSUM){
            printed.append("The checksum does not match the expected value");
        } else if(masked == CYBTLDR_STAT_ERR_ARRAY){
            printed.append("The flash array is not valid");
        } else if(masked == CYBTLDR_STAT_ERR_ROW){
            printed.append("The flash row is not valid");
        } else if(masked == CYBTLDR_STAT_ERR_PROTECT){
            printed.append("The bootloader is not ready to process data");
        } else if(masked == CYBTLDR_STAT_ERR_APP){
            printed.append("The application is currently marked as active");
        } else if(masked == CYRET_ERR_UNK){
            printed.append("An unknown error occurred");
        }
    } else {
        printed.append("Programmed Successfully");
    }


    return printed;
}
#endif

/* Module class code */

Module::Module(uint8_t address){
    this->address = address;
    this->position_ = 0;
    this->velocity_ = 0;
    this->effort_ = 0;
    this->encoder_status = 0;

    this->last_position_cmd = NAN;
    this->last_velocity_cmd = NAN;
    this->last_effort_cmd = NAN;

    this->firmware_version = "N/A";
    this->firmware_build_name = "N/A";
    this->firmware_tag = "N/A";
    this->firmware_build_time = "N/A";

    this->t_last_update = current_time_ms();
    this->stale = false;

    this->command_support = 0;
    this->UID = 0; //The actual UID of a chip being 0 should be next to impossible

    this->sync_ind = 0;
    this->sync_reg = 0;
    this->status_code = 0;
    this->module_status = "N/A";
    for(uint8_t i = 0; i < MAX_LED_CODE_LEN + 1; i++){
        this->LED_code[i] = 0;
    }
    this->t_sync = current_time_ms();
    this->last_debug_msg = "N/A";

    this->t_last_param_update = current_time_ms();
}

Module::~Module(){

}

void Module::update_parameter(uint8_t param, uint64_t value){
    std::lock_guard<std::mutex> lock(mutex);

    if(param >= param_list.size()) return;
    param_list[param].value = value;

    if(param == TYPE_PARAM){
        this->type = value;
    }
}

uint64_t Module::read_parameter(uint8_t param){
    std::lock_guard<std::mutex> lock(mutex);

    if(param >= param_list.size()) return -1;
    uint64_t retval = param_list[param].value;

    return retval;
}

std::string Module::print_parameter(uint8_t param) const{
    std::lock_guard<std::mutex> lock(mutex);
    if(param > param_list.size()) return "ERR\n";

    uint8_t s[32];

    module_param mod_param = param_list[param];
    switch(mod_param.type){
    case _UINT8: {
        snprintf((char *)s, 32, "%u\n", (uint8_t)mod_param.value);
        break;
    } case _UINT16: {
        snprintf((char *)s, 32, "%u\n", (uint16_t)mod_param.value);
        break;
    } case _UINT32: {
        snprintf((char *)s, 32, "%u\n", (uint32_t)mod_param.value);
        break;
    } case _UINT64: {
        snprintf((char *)s, 32, "%llu\n", mod_param.value);
        break;
    } case _FLOAT: {
        snprintf((char *)s, 32, "%08.4f\n", *(float *)(&(mod_param.value)));
        break;
    } case _DOUBLE: {
        snprintf((char *)s, 32, "%08.4f\n", *(double *)(&(mod_param.value)));
        break;
    } default: {
        snprintf((char *)s, 32, "ERR\n");
        break;
    }
    }

    return std::string((char *)s);
}

void Module::add_parameter(uint8_t id, uint8_t type, std::string name){
    std::lock_guard<std::mutex> lock(mutex);

    module_param null_param;
    null_param.value = 0;
    null_param.dirty = 0;
    null_param.type = 0;
    null_param.name = "";

    if(id >= param_list.size()){
        //If the ID is past the end, resize to include it
        param_list.resize(id+1, null_param);
    }

    if(param_list[id].type == 0)
        received_params++;

    module_param param;
    param.value = 0;
    param.dirty = 0;
    param.type = type;
    param.name = name;

    //Update the param list
    param_list[id] = param;
}

std::string Module::parameter_name(uint8_t id) const{
    std::lock_guard<std::mutex> lock(mutex);
    if(id >= param_list.size()) return "ERR";

    std::string retval = param_list[id].name;
    return retval;
}

void Module::set_param_last_update(){
    t_last_param_update = current_time_ms();
}

void Module::set_expected_parameters(uint8_t num_parameters){
    this->expected_num_params = num_parameters;
}

uint8_t Module::parameters_left() const{
    //TODO: Some sort of error fixing here. If this is wrong, we need to re check everything
    if(received_params > expected_num_params) return 0;

    return expected_num_params - received_params;
}

uint64_t Module::d_t_param_last_update() const{
    return current_time_ms() - t_last_param_update;
}

void Module::add_downstream(uint8_t node_addr){
    std::lock_guard<std::mutex> lock(mutex);

    module_down_port port;
    port.addr_current = node_addr;
    port.addr_diff = node_addr;
    port.consistency_count = 0;
    downstream_list.push_back(port);
}

uint8_t Module::update_downstream(uint8_t ind, uint8_t node_addr){
    std::lock_guard<std::mutex> lock(mutex);

    module_down_port empty_port;
    empty_port.addr_current = 0xFF;
    empty_port.addr_diff = 0xFF;
    empty_port.consistency_count = 0;
    empty_port.name = "N/A";

    //If the list does not have an entry yet, add one
    if(ind >= downstream_list.size()){
        downstream_list.resize(ind + 1, empty_port);
    }

    //If the port name is invalid, ask for the name again
    if(downstream_list[ind].name == "N/A"){
        firmware_utility(address, EIGEN_UTIL_MODULE_PORTS);
    }

    module_down_port downstream = downstream_list[ind];
    uint8_t retval = 0;
    //If the address differs from what we currently have stored, we have to do some more checks
    if(node_addr != downstream.addr_current){
        //If this is the first time we see this new value, then mark the consistency count as 0
        if(node_addr != downstream.addr_diff){
            downstream.addr_diff = node_addr;
            downstream.consistency_count = 0;
        } else {
            //If this is not the first time we have seen this value, then increment the counter
            downstream.consistency_count++;

            //If we have seen the value enough times, then update the struct
            if(downstream.consistency_count == TOPOLOGY_CONSISTENCY_COUNT){
                downstream.addr_current = node_addr;
                downstream.addr_diff = node_addr;
                retval = 1;
            } else {
                //If we haven't seen the value enough times yet, keep checking to make sure it is correct
                add_command(address, CMD_POLL_TOPOLOGY, 0 /* Unused Value */);
            }
        }
        downstream_list[ind] = downstream;
    }

    return retval;
}

void Module::clear_downstream(){
    std::lock_guard<std::mutex> lock(mutex);
    downstream_list.clear();
}

void Module::set_downstream_name(uint8_t ind, std::string name){
    std::lock_guard<std::mutex> lock(mutex);

    module_down_port empty_port;
    empty_port.addr_current = 0xFF;
    empty_port.addr_diff = 0xFF;
    empty_port.consistency_count = 0;
    empty_port.name = "N/A";

    //If the list does not have an entry yet, add one
    if(ind >= downstream_list.size()){
        downstream_list.resize(ind + 1, empty_port);
    }

    module_down_port downstream = downstream_list[ind];
    downstream.name = name;
    downstream_list[ind] = downstream;
}

/* print_topology:
 * Prints out a list of downstream modules in the following format:
 * T<address>,<downstream 1>,...,<downstream n>\n
*/
std::string Module::print_topology() const{
    std::lock_guard<std::mutex> lock(mutex);
    uint8_t s[128];
    uint8_t offset = 0;

    //Print the header
    offset += snprintf((char *)(s + offset), 128 - offset, " {\"id\":\"%02X\", \"type\":\"%02X\", \"orientation\":\"%02X\", \"children\":[", address, type, orientation);

    //Print the body
    for(auto item : downstream_list){
        offset += snprintf((char *)(s + offset), 128 - offset, "\"%02X\",", item.addr_current);
    }

    //Demarcate the end
    offset += snprintf((char *)(s + offset - 1), 128 - offset, "]}, ");

    return std::string((char *)s);
}

void Module::update_UID(uint64_t UID_){
    std::lock_guard<std::mutex> lock(mutex);

    if(UID == 0){ //If this is the first time we have seen a UID
        UID = UID_;
    } else if(UID != UID_){ //If we already have a UID and this one doesn't match, there must be a conflict.
        add_command(this->address, CMD_UID_WR_ADDR, UID_); //Add a command to resolve this conflict
    }
}

uint64_t Module::get_UID() const{
    return UID;
}

void Module::update_type(uint8_t type_){
    std::lock_guard<std::mutex> lock(mutex);

    type = type_;
}

void Module::update_orientation(uint8_t orientation_){
    std::lock_guard<std::mutex> lock(mutex);

    orientation = orientation_;
}

std::vector<module_down_port> Module::downstream() const{
    std::lock_guard<std::mutex> lock(mutex);
    return downstream_list;
}

uint8_t Module::parameter_type(uint8_t id) const{
    std::lock_guard<std::mutex> lock(mutex);
    if(id >= param_list.size()) return 0;

    uint8_t retval = param_list[id].type;
    return retval;
}

std::string Module::print_mod_name() const{
    std::lock_guard<std::mutex> lock(mutex);
    uint8_t s[32];

    snprintf((char *)s, 32, "Module %d", address);
    return std::string((char *)s);
}

std::string Module::print_UID() const{
    std::lock_guard<std::mutex> lock(mutex);
    uint8_t s[32];

    snprintf((char *)s, 32, "u%016llX", UID);
    return std::string((char *)s);
}

std::string Module::print_type() const{
    std::lock_guard<std::mutex> lock(mutex);
    uint8_t s[32];

    snprintf((char *)s, 32, "Type: %d", type);
    return std::string((char *)s);
}

uint8_t Module::get_type() const{
    std::lock_guard<std::mutex> lock(mutex);
    return type;
}

uint8_t Module::get_hardware_type() const{
    std::lock_guard<std::mutex> lock(mutex);

    switch(type){
        case NODE_TYPE_WHEEL:           return HARDWARE_EIGEN;
        case NODE_TYPE_TWIST:           return HARDWARE_EIGEN;
        case NODE_TYPE_BEND:            return HARDWARE_EIGEN;
        case NODE_TYPE_GRIPPER_FOOT:    return HARDWARE_EIGEN;
        case NODE_TYPE_GRIPPER:         return HARDWARE_EIGEN;
        case NODE_TYPE_O_6:             return HARDWARE_O6;
        case NODE_TYPE_BATTERY:         return HARDWARE_MISC;
        case NODE_TYPE_EIGENBODY:       return HARDWARE_HUB_9;
        case NODE_TYPE_TEE:             return HARDWARE_EIGEN;
        case NODE_TYPE_FOOT:            return HARDWARE_EIGEN;
        case NODE_TYPE_STAT_NO_BEND:    return HARDWARE_EIGEN;
        case NODE_TYPE_STAT_45_BEND:    return HARDWARE_EIGEN;
        case NODE_TYPE_STAT_90_BEND:    return HARDWARE_EIGEN;
        case NODE_TYPE_HUB_9:           return HARDWARE_HUB_9;
        default:                        return HARDWARE_MISC;
    }
}

std::string Module::print_orientation() const{
    std::lock_guard<std::mutex> lock(mutex);
    uint8_t s[32];

    snprintf((char *)s, 32, "Orientation: %d", orientation);
    return std::string((char *)s);
}

uint16_t Module::get_encoder_status() const{
    std::lock_guard<std::mutex> lock(mutex);
    return this->encoder_status;
}

void Module::set_encoder_status(uint16_t status){
    std::lock_guard<std::mutex> lock(mutex);
    this->encoder_status = status;
}

double Module::position() const{
    std::lock_guard<std::mutex> lock(mutex);
    return this->position_;
}

double Module::velocity() const{
    std::lock_guard<std::mutex> lock(mutex);
    return this->velocity_;
}

double Module::effort() const{
    std::lock_guard<std::mutex> lock(mutex);
    return this->effort_;
}

void Module::set_position(double position){
    std::lock_guard<std::mutex> lock(mutex);
    this->position_ = position;
}

void Module::set_velocity(double velocity){
    std::lock_guard<std::mutex> lock(mutex);
    this->velocity_ = velocity;
}

void Module::set_effort(double effort){
    std::lock_guard<std::mutex> lock(mutex);
    this->effort_ = effort;
}

uint8_t Module::get_address() const{
    std::lock_guard<std::mutex> lock(mutex);
    return this->address;
}

std::string Module::print_encoder_status() const{
    if(encoder_status == 00)                    return "Working Correctly";

    if(encoder_status & ENC_ACCELERATION_ERROR) return "Acceleration Error";
    if(encoder_status & ENC_MAG_PATTERN_ERROR)  return "Magnetic Pattern Error";
    if(encoder_status & ENC_SYSTEM_ERROR)       return "System Error";
    if(encoder_status & ENC_SUPPLY_ERROR)       return "Supply Error";

    if(encoder_status & ENC_TEMP_WARNING)       return "Temperature Warning";
    if(encoder_status & ENC_SIG_LOST_ERROR)     return "Signal Lost Error";
    if(encoder_status & ENC_SIG_AMP_LOW_WARN)   return "Signal Amplitude Low";
    if(encoder_status & ENC_SIG_AMP_HIGH_WARN)  return "Signal Amplitude High";

    if(encoder_status & ENC_OP_LIMITS_WARNING)  return "Output Limits Warning";
    if(encoder_status & ENC_DATA_INVALID_ERROR) return "Data Invalid Error";
    if(encoder_status & ENC_ENC_CONFIG_ERROR)   return "Config Error";
    if(encoder_status & ENC_SENSOR_READ_ERROR)  return "Sensor Read Error";

    if(encoder_status & ENC_MAG_SENSOR_ERROR)   return "Magnetic Sensor Error";
    if(encoder_status & ENC_SIG_AMP_WARNING)    return "Signal Amplitude Warning";
    if(encoder_status & ENC_SIG_AMP_ERROR)      return "Signal Amplitude Error";
    if(encoder_status & ENC_COUNTER_ERROR)      return "Counter Error";
}

std::vector<ModuleShared> *get_module_list(){
    return &module_list;
}

/* Eigen Packet Class Definitions */
//TODO: What to do when we expect multiple responses to a packet, and do not know how many?
EigenPacketFilter::EigenPacketFilter(uint8_t address, std::string response_filter,
                                     packet_type packet, std::string packet_string){
    this->address = address;
    this->response_filter = response_filter;
    this->t_sent = current_time_ms();
    this->classification = packet;
    this->packet_string_ = packet_string;
    this->retries = 0;
}

EigenPacketFilter::~EigenPacketFilter(){
    this->matched_responses.clear();
    this->matched_addresses.clear();
}

bool EigenPacketFilter::expects_response(){
    return response_filter != "";
}

uint8_t EigenPacketFilter::num_responses(){
    //Should not get more than 255 responses to a single request. If we do, there is something wrong
    return matched_responses.size();
}

bool EigenPacketFilter::matches_filter(uint8_t address, std::string packet){
    if(!is_broadcast() && address != this->address) return false;
    if(matched_addresses.count(address) == 0){
        if(packet.find(response_filter) != std::string::npos){
            return true;
        }
    }
    return false;
}

void EigenPacketFilter::add_response(uint8_t address, std::string response){
    //Change to end of list for better efficiency?
    matched_responses.insert(matched_responses.begin(), response);
    matched_addresses.insert(address);
}

bool EigenPacketFilter::packet_timeout(){
    return (current_time_ms() - t_sent) > PACKET_TIMEOUT;
}

bool EigenPacketFilter::is_broadcast(){
    return address == 0xFF;
}

packet_type EigenPacketFilter::get_type(){
    return this->classification;
}

uint8_t EigenPacketFilter::num_retries(){
    return retries;
}

void EigenPacketFilter::increment_retry_count(){
    retries++;
}

void EigenPacketFilter::reset_timeout(){
    t_sent = current_time_ms();
}

std::string EigenPacketFilter::packet_string(){
    return this->packet_string_;
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
