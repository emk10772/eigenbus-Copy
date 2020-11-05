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
static std::vector<std::shared_ptr<EigenModule>> module_list;
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
    /*stats.uptime_ms = current_time_ms() - t_init;
    stats.sent_packets = sent_packets;
    stats.successful_packets = successful_packets;
    stats.dropped_packets = dropped_packets;
    stats.unrequested_packets = unrequested_packets;
    stats.retried_packets = retried_packets;
    stats.frame_time_ms = frame_time;
    stats.last_dropped_packet = last_dropped;*/
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
        packet_type type = packetTracker->match_response(addr, std::string((char *) buffer + 1));
        
        //Parse the response
        EigenResponse *response = packetParser->parse_packet(addr, std::string((char *) buffer + 3));
        if(response->update_module(module)){
            add_module_update(addr, response->update_type());
        }

        if(response->has_additonal_responses()){
            //Add more packets to the tracker
            for(auto pkt : response->additional_responses())
                packetTracker->add_packet(module->get_address(), pkt, "", type);
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
