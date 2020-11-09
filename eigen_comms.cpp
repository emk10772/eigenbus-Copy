#include "eigen_comms.h"
#include "eigen_command_wrappers.h"
#include "eigen_bootloader.h"
#include <stdlib.h>
#include <ctype.h>
#include <algorithm>
#include <deque>
#include <stdio.h>
#include <string.h>
#include <cmath>

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

static uint64_t frame_time = 0;

//Module data structure
static std::vector<std::shared_ptr<EigenModule>> module_list;
std::mutex module_list_mutex;

//Interface deques and mutexes for thread safety
static std::deque<EigenUpdate *> update_list;
static std::mutex update_mutex;
static std::deque<EigenCommand *> cmd_list;
static std::mutex cmd_mutex;

static EigenPacketTracker *packetTracker;
static EigenPacketParser *packetParser;
static EigenBootloader *bootloader;

static std::deque<std::string> packet_queue;

static bool list_update;
static bool enabled;
static eigen_config communication_config;


/* Private forward declarations */
void clear_module_list();
EigenCommand *get_command();
void add_module_update(EigenUpdate *update);
uint8_t generate_node_address();

//Module list interface functions
ModuleShared add_module(uint8_t address);
ModuleShared get_module_shared(uint8_t address);
ModuleShared get_module_shared_by_index(uint8_t index);

void write_packet(const char *buf, uint8_t len){
    static char s[256] = {0};

    if(len == 0) return;

    uint8_t crc = crc_8_ccitt(buf, len);

    int count = std::snprintf(s, 256, "%s:%02X\n", buf, crc);
    (*write_data)((uint8_t *)s, count);
}

eigen_stats get_eigen_stats(){
    eigen_stats stats;
    stats.uptime_ms = current_time_ms() - t_init;
    stats.sent_packets = packetTracker->packets_sent;
    stats.successful_packets = packetTracker->successful_packets;
    stats.dropped_packets = packetTracker->packets_dropped;
    stats.unrequested_packets = packetTracker->unrequested_packets;
    stats.retried_packets = packetTracker->retried_packets;
    stats.frame_time_ms = frame_time;
    stats.last_dropped_packet = packetTracker->last_packet_dropped;
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
    srand(t_init);

    bootloader = EigenBootloader::getInstance();
    packetTracker = new EigenPacketTracker();
    packetParser = new EigenPacketParser();

}

void clean_eigen_comms() {
    clear_module_list();
    delete packetParser;
    delete packetTracker;
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
            add_module_update(new EigenUpdate((*mod_it)->get_address(), EigenUpdate::MODULE_REMOVED));

            mod_it = module_list.erase(mod_it);
        } else {
            ++mod_it;
        }
    }
#endif
}

void add_module_update(EigenUpdate *update){
    std::lock_guard<std::mutex> lock(update_mutex);

    update_list.push_back(update);
}

EigenUpdate *get_module_update(){
    EigenUpdate *retval;
    std::lock_guard<std::mutex> lock(update_mutex);

    if(update_list.size() > 0){
        retval = update_list.front();
        update_list.pop_front();
    } else {
        return nullptr;
    }

    return retval;
}

bool is_bootloader_active(){
    return bootloader->active();
}

bool is_bootloader_finished(){
    return bootloader->finished();
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
            uint8_t chk_calc = crc_8_ccitt((char *)buffer, len-3);
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
        packet_type type = packetTracker->match_response(addr, std::string((char *) buffer + 1));
        
        //Parse the response
        EigenResponse *response = packetParser->parse_packet(addr, std::string((char *) buffer + 3));
        if(response != nullptr){
            EigenUpdate *update = response->update_module(module);
            if(update != nullptr)
                add_module_update(update);

            if(response->has_additonal_responses())
                for(auto pkt : response->additional_responses())
                    packetTracker->add_packet(module->get_address(), pkt, "", type);

            bootloader->process_packet(response);
        }
        
        delete response;
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

raw_packet *get_raw_packet(){
    return packetTracker->get_raw_packet();
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

void service_bootloader(){
    EigenCommand *cmd;
    cmd = bootloader->get_command();

    while(cmd != NULL){
        write_packet(cmd->packet().c_str(), cmd->packet().length());

        delete cmd;
        cmd = bootloader->get_command();
    }

    EigenUpdate *update = bootloader->get_update();
    while(update != NULL){
        add_module_update(update);
        update = bootloader->get_update();
    }
}

int service_eigen_comms() {
    static uint64_t t_last = current_time_ms();

    uint8_t packet_count = 0;

    if(!enabled) return 0;

    frame_time = current_time_ms() - t_last;
    t_last = current_time_ms();

    packetTracker->handle_timeout_packets();

    if(!bootloader->active()){
        //Execute any queued commands
        EigenCommand *cmd = get_command();

        while(cmd != NULL){
            write_packet(cmd->packet().c_str(), cmd->packet().length());

            packetTracker->add_packet(cmd->address(), cmd->expected_response(), cmd->packet(), cmd->type());
            bootloader->process_command(cmd);

            delete cmd;
            cmd = get_command();
        }
    }

    service_bootloader();

    /* POLLING */
    if(current_time_ms() - t_last_update_poll > UPDATE_TOPOLOGY_PERIOD){
        eigen_poll_topology(0xFF);
        t_last_update_poll = current_time_ms();
    }

    if(current_time_ms() - t_last_status_poll > UPDATE_STATUS_PERIOD){
        //Poll module statuses
        eigen_firmware_utility(0xFF, EIGEN_UTIL_STAT_CODE);
        t_last_status_poll = current_time_ms();
    }

    //If we are due to poll, communicate with the module
    if (current_time_ms() - t_last_poll > UPDATE_PERIOD) {
        if(communication_config.poll_encoder_status == EIGEN_ENABLED)
            eigen_poll_status(0xFF, EIGEN_POLL_ENC_STATUS);
        if(communication_config.poll_module_position == EIGEN_ENABLED)
            eigen_poll_status(0xFF, EIGEN_POLL_LOCATION);
        if(communication_config.poll_module_velocity == EIGEN_ENABLED)
            eigen_poll_status(0xFF, EIGEN_POLL_VELOCITY);
        if(communication_config.poll_module_effort == EIGEN_ENABLED)
            eigen_poll_status(0xFF, EIGEN_POLL_EFFORT);

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
        ModuleShared mod = get_module_shared_by_index(ind);
        if(mod->parameters.parameters_left() > 0 && mod->parameters.d_t_last_update() > PACKET_TIMEOUT){
            eigen_read_parameter(mod->get_address(), LIST_PARAM);
            mod->parameters.set_last_update();
        }
    }

    //Handle the successful packets
    packetTracker->handle_successful_packets();

    //Mark the stale modules as such
    clean_module_list();

    return packet_count;
}

void add_command(EigenCommand *command){
    std::lock_guard<std::mutex> lock(cmd_mutex);
    cmd_list.push_back(command);
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

bool mod_cmp(EigenModule *m1, EigenModule *m2){
    return (m1->get_address()) < (m2->get_address());
}

bool mod_cmp_low_bnd(EigenModule *m1, uint8_t addr){
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

ModuleShared get_module_shared_by_index(uint8_t index){
    std::lock_guard<std::mutex> lock(module_list_mutex);

    if(index >= module_list.size()) return NULL;

    return module_list[index];
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
        mod_result = std::make_shared<EigenModule>(address);
        module_list.push_back(mod_result);
        //std::sort(module_list.begin(), module_list.end(), mod_cmp);

        //Log that we updated the list
        list_update = true;
        add_module_update(new EigenUpdate(address, EigenUpdate::MODULE_ADDED));

        //Ask for important info about the module
        eigen_firmware_utility(address, EIGEN_UTIL_COMMIT_VERSION);
        eigen_firmware_utility(address, EIGEN_UTIL_BUILD_TIME);
        eigen_firmware_utility(address, EIGEN_UTIL_BUILD_USER);
        eigen_firmware_utility(address, EIGEN_UTIL_GIT_DESCRIBE);
        eigen_firmware_utility(address, EIGEN_UTIL_MODULE_CAPABILITY);
        eigen_firmware_utility(address, EIGEN_UTIL_MODULE_PORTS);
        eigen_firmware_utility(address, EIGEN_UTIL_MODULE_UID);
        eigen_firmware_utility(address, EIGEN_UTIL_MODULE_STATUS);
    } else {
        add_module_update(new EigenUpdate(address, EigenUpdate::MODULE_TOUCHED));
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
        return std::const_pointer_cast<const EigenModule>(mod_result);
    }

}

ModuleConst get_module_by_index(uint8_t index){
    std::lock_guard<std::mutex> lock(module_list_mutex);

    if(index >= module_list.size()) return NULL;

    ModuleShared mod = module_list[index];
    return std::const_pointer_cast<const EigenModule>(mod);
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
