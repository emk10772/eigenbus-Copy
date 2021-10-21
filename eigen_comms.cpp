#include "eigen_comms.h"
#include "eigen_command_wrappers.h"
#include "eigen_bootloader.h"
#include "eigen_packet_poll.h"
#include "eigen_vector_map.h"
#include "eigen_topology_tracker.h"
#include <stdlib.h>
#include <ctype.h>
#include <algorithm>
#include <deque>
#include <map>
#include <stdio.h>
#include <string.h>
#include <cmath>

#define IN_BUFFER_SIZE          (255)

#define ADDR_PARAM              (1)
#define TYPE_PARAM              (2)

//Private Variables
static uint16_t (*read_data)(uint8_t *buf, uint16_t max_len, int t_wait_ms);
static void (*write_data)(uint8_t *buf, uint16_t len);

static uint64_t t_init = 0;
static uint64_t frame_time = 0;
static uint64_t t_last_slow_cmd = 0;

//Data structures
static EigenVectorMap module_list_;
static EigenVectorMap module_list_pub_;
static EigenQueue<EigenUpdate> update_list_;
static EigenQueue<EigenCommand> cmd_list_;
static EigenQueue<EigenCommand> cmd_list_slow_;
static std::vector<ModuleShared> module_init_list_;

//Packet processing classes
static EigenPacketTracker *packetTracker;
static EigenPacketParser *packetParser;
static EigenBootloader *bootloader;
static EigenPacketPoll *packetPoll;
static EigenTopologyTracker *topologyTracker;

static std::deque<std::string> packet_queue;

static bool list_update;
static bool enabled;
static eigen_config communication_config;


/* Private forward declarations */
void clear_module_list();
EigenCommand *get_command();
void add_module_update(EigenUpdate *update);
uint8_t generate_node_address();
void read_module_params(ModuleShared module);
void read_module_mailboxes(ModuleShared module);

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

    stats.sent_packets = packetTracker->packets_sent.total();
    stats.sent_rate = packetTracker->packets_sent.rate();

    stats.successful_packets = packetTracker->successful_packets.total();
    stats.success_rate = packetTracker->successful_packets.rate();

    stats.dropped_packets = packetTracker->packets_dropped.total();
    stats.dropped_rate = packetTracker->packets_dropped.rate();

    stats.unrequested_packets = packetTracker->unrequested_packets.total();
    stats.unrequested_rate = packetTracker->unrequested_packets.rate();

    stats.retried_packets = packetTracker->retried_packets.total();
    stats.retried_rate = packetTracker->retried_packets.rate();

    stats.spontaneous_packets = packetTracker->spontaneous_packets.total();
    stats.spontaneous_rate = packetTracker->spontaneous_packets.rate();

    stats.frame_time_ms = frame_time;
    stats.last_dropped_packet = packetTracker->last_packet_dropped;
    stats.avg_latency = packetTracker->avg_latency();
    stats.peak_latency = packetTracker->peak_latency();
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
    t_last_slow_cmd = current_time_ms();
    srand(t_init);

    bootloader = EigenBootloader::getInstance();
    packetTracker = new EigenPacketTracker();
    packetParser = new EigenPacketParser();
    topologyTracker = new EigenTopologyTracker;

    // Setup polling rules
    packetPoll = new EigenPacketPoll();
    packetPoll->add_command(new EigenCommandTopology(0xFF), UPDATE_TOPOLOGY_PERIOD, true);
    packetPoll->add_command(new EigenCommandUtility(0xFF, EIGEN_UTIL_STAT_CODE), UPDATE_STATUS_PERIOD, true);

}

void clean_eigen_comms() {
    clear_module_list();
    delete packetParser;
    delete packetPoll;
    delete packetTracker;
    delete topologyTracker;
}

void clear_module_list(){
    module_list_.clear();
}

void add_module_update(EigenUpdate *update){
    update_list_.add(update);
    topologyTracker->add_update(update);
}

void add_module_update(std::vector<EigenUpdate *> updates){
    update_list_.add(updates);
    topologyTracker->add_update(updates);
}

EigenUpdate *get_module_update(){
    return update_list_.get();
}

bool is_bootloader_active(){
    return bootloader->active();
}

bool is_bootloader_finished(){
    return bootloader->finished();
}

uint8_t generate_node_address(){
    //Not a very efficient implementation, but should be fine because we will use this very rarely
    std::set<eigen_addr_t> occupied_addrs = module_list_.keys();

    //Randomly try to find a free address
    uint8_t addr = rand() % 0xFF;
    while(occupied_addrs.count(addr) != 0){
        addr = rand() % 0xFF;
    }

    //TODO: Check this
    return addr;
}

void process_packet(uint8_t *buffer, uint8_t len) {
    eigen_addr_t addr = 0;
    uint8_t *ptr = buffer;
    
    if (len > 3 && buffer[0] == '.' &&
        isxdigit(buffer[1]) && isxdigit(buffer[2])) { //Feedback messages have periods

        uint8_t pkt_valid = 1;
        //Check for a valid address
        addr = (eigen_addr_t) strtol((char *)buffer+1, (char **)&ptr, 16);
        if (ptr - buffer != 3){
            pkt_valid = 0;
        }

        //Check the checksum. Should start at len - 3 if this is a valid packet
        if(buffer[len-3] == ':'){
            uint8_t chk_read = (uint8_t) strtol((char *)buffer + len - 2, NULL, 16);
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
            return;
        }

        //If the address is valid add it to the list
        ModuleShared module = add_module(addr);

        //Parse the response
        EigenResponse *response = packetParser->parse_packet(addr, std::string((char *) buffer + 3));

        if(response != nullptr){
            //Check if the response matches one that we are looking for
            response_match_t retval = packetTracker->match_response(module, response, std::string((char *) buffer + 1));

            //Get the update and add it to the update queue if it exists
            EigenUpdate *update = response->update_module(module, retval.second);
            if(update != nullptr)
                add_module_update(update);

            //If we expect more responses after this one, notify the packet tracker
            if(response->has_additonal_responses())
                for(auto &pkt : response->additional_responses())
                    packetTracker->add_packet(module->address, pkt, "", retval.first);

            //Allow the bootloader to process the packets
            bootloader->process_packet(response);
        } else {
            //If we could not parse the response, send it to the tracker to record the error
            packetTracker->match_response(module, nullptr, std::string((char *) buffer + 1));
        }
        
        delete response;
    } else {

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

std::string add_poll_command(EigenCommand *command, uint64_t period_ms, bool enabled){
    return packetPoll->add_command(command, period_ms, enabled);
}

void set_poll_enable(std::string key, bool enabled){
    packetPoll->set_enable(key, enabled);
}

void remove_poll_command(std::string key){
    packetPoll->remove_command(key);
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

    //Write everything from the bootloader command queue
    while(cmd != NULL){
        write_packet(cmd->packet().c_str(), cmd->packet().length());
        packetTracker->add_packet(cmd->address(), cmd->expected_response(), cmd->packet(), cmd->type());

        delete cmd;
        cmd = bootloader->get_command();
    }

    //Deal with any module updates
    EigenUpdate *update = bootloader->get_update();
    while(update != NULL){
        add_module_update(update);
        update = bootloader->get_update();
    }
}

int service_eigen_comms() {
    static uint64_t t_last = current_time_ms();
    static uint8_t ind = 0;
    static uint64_t t_last_poll = current_time_ms();

    uint8_t packet_count = 0;

    if(!enabled) return 0;

    frame_time = current_time_ms() - t_last;
    t_last = current_time_ms();

    //Deal with any packets that have timed out
    packetTracker->handle_timeout_packets();
    while(packetTracker->packets_out.size() > 0){
        std::string pkt = packetTracker->packets_out.front();
        write_packet(pkt.c_str(), pkt.length());
        packetTracker->packets_out.pop_front();
    }

    if(!bootloader->active()){
        //See if we should execute a command from our slow command list
        //Used for commands that result in a large demand on the serial bus, i.e. param list reads
        EigenCommand *cmd = nullptr;
        if(current_time_ms() - t_last_slow_cmd > SLOW_CMD_PERIOD){
            EigenCommand *slow_cmd = cmd_list_slow_.get();
            if(slow_cmd) {
                t_last_slow_cmd = current_time_ms();
                cmd = slow_cmd;
            }
        }

        //If there is no command from the slow list, get a command from the normal list
        if(!cmd)
            cmd = get_command();

        //Execute the command and go through remaining commands in the queue
        while(cmd != nullptr){
            ModuleShared mod = get_module_shared(cmd->address());
            write_packet(cmd->packet().c_str(), cmd->packet().length());
            if(mod) cmd->update_module(mod);

            packetTracker->add_packet(cmd->address(), cmd->expected_response(), cmd->packet(), cmd->type());
            bootloader->process_command(cmd);

            delete cmd;
            cmd = get_command();
        }

        //Service the polling manager
        packetPoll->service_poll();

        //Check that the module's parameters are updated properly
        if(current_time_ms() - t_last_poll > 50){
            t_last_poll = current_time_ms();

            //Check up on our initializing modules
            auto it = module_init_list_.begin();
            while(it != module_init_list_.end()){
                ModuleShared mod = *it;
                if(mod->valid()){
                    //If the module is finished initializing, remove it from the init list
                    module_list_pub_.insert(mod);
                    list_update = true;
                    add_module_update(new EigenUpdate(mod->address, EigenUpdate::MODULE_ADDED, mod));
                    it = module_init_list_.erase(it);
                } else {
                    if(mod->parameters.update_required()){
                        read_module_params(mod);
                    }
                    if(mod->mailboxes.update_required()){
                        read_module_mailboxes(mod);
                    }
                    it++;
                }
            }
        }
    }

    service_bootloader();

    //If there are commands from the poll manager, add them to the queue
    cmd_list_.add(packetPoll->get_commands());

    //Process any packets that are in the queue
    packet_count = parse_packets(0, NULL);
    while(packet_queue.size() > 0){
        std::string packet = packet_queue.front();
        packet_queue.pop_front();
        process_packet((uint8_t *)packet.c_str(), packet.size());
    }

    //Handle the successful packets
    packetTracker->handle_successful_packets();

    //Remove modules we haven't heard from recently
    add_module_update(module_list_pub_.clear_old(current_time_ms(), 2*UPDATE_TOPOLOGY_PERIOD));
    module_list_.clear_old(current_time_ms(), 2*UPDATE_TOPOLOGY_PERIOD);

    //Process any updates before returning
    //Otherwise the interface might delete the updates first
    topologyTracker->process_updates();

    return packet_count;
}

void add_command(EigenCommand *command){
    cmd_list_.add(command);
}

void clear_commands(){
    cmd_list_.clear();
}

EigenCommand *get_command(){
    return cmd_list_.get();
}

ModuleShared get_module_shared(uint8_t address){
    return module_list_.get_shared(address);
}

ModuleShared get_module_shared_by_index(uint8_t index){
    return module_list_.get_shared_by_ind(index);
}

ModuleShared add_module(uint8_t address){
    ModuleShared mod_result = get_module_shared(address);

    if(mod_result == NULL || mod_result->address != address){
        mod_result = std::make_shared<EigenModule>(address);
        module_list_.insert(mod_result);
        module_init_list_.emplace_back(mod_result);

        //Ask for important info about the module
        eigen_firmware_utility(address, EIGEN_UTIL_COMMIT_VERSION);
        eigen_firmware_utility(address, EIGEN_UTIL_BUILD_TIME);
        eigen_firmware_utility(address, EIGEN_UTIL_BUILD_USER);
        eigen_firmware_utility(address, EIGEN_UTIL_GIT_DESCRIBE);
        eigen_firmware_utility(address, EIGEN_UTIL_MODULE_CAPABILITY);
        eigen_firmware_utility(address, EIGEN_UTIL_MODULE_PORTS);
        eigen_firmware_utility(address, EIGEN_UTIL_MODULE_UID);
        eigen_firmware_utility(address, EIGEN_UTIL_MODULE_STATUS);
        eigen_firmware_utility(address, EIGEN_UTIL_MODULE_NAME);

        read_module_params(mod_result);
        read_module_mailboxes(mod_result);
    } else {
        add_module_update(new EigenUpdate(address, EigenUpdate::MODULE_TOUCHED, mod_result));
    }
    mod_result->t_last_update = current_time_ms();
    mod_result->stale = false;

    return mod_result;
}

void read_module_params(ModuleShared module){
    cmd_list_slow_.add(new EigenCommandParamRead(module->address, LIST_PARAM));
    module->parameters.set_request_in_progress(true);
    module->parameters.set_last_update();
}

void read_module_mailboxes(ModuleShared module){
    cmd_list_slow_.add(new EigenCommandMailboxRead(module->address, LIST_PARAM));
    module->mailboxes.set_request_in_progress(true);
    module->mailboxes.set_last_update();
}

ModuleConst get_module(eigen_addr_t address){
    return module_list_pub_.get_const(address);
}

ModuleConst get_module_by_index(eigen_addr_t index){
    return module_list_pub_.get_const_by_ind(index);
}

std::vector<ModuleConst> get_modules(std::vector<eigen_addr_t> addresses) {
    std::vector<ModuleConst> retval;

    //Populate a list with modules
    for(eigen_addr_t addr : addresses){
        auto module = get_module(addr);
        if(module != nullptr)
            retval.emplace_back(module);
    }

    return retval;
}

uint8_t num_modules(){
    return module_list_pub_.size();
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

const EigenTopologyTracker *eigen_topology(){
    return topologyTracker;
}

