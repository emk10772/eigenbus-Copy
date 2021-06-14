#include "eigen_module.h"
#include "eigen_comms.h"

/* EigenModule class code */

EigenModule::EigenModule(uint8_t address){
    //Initialize our list with pointers to all of our class variables
    this->variable_list.resize(EIGEN_NUM_VARIABLES, nullptr);
#define ENTRY(e_name, type, v_name) this->variable_list[e_name] = &this->v_name;
    VAR_LIST
#undef ENTRY
    //Set the IDs for each variable
#define ENTRY(e_name, type, v_name)     \
    this->v_name.set_id(e_name);        \
    this->v_name.set_address(address);  \
    VAR_LIST
#undef ENTRY
    this->address = address;

    this->last_position_cmd = nan("");
    this->last_velocity_cmd = nan("");
    this->last_effort_cmd = nan("");

    this->firmware_version = "N/A";
    this->firmware_build_name = "N/A";
    this->firmware_tag = "N/A";
    this->firmware_build_time = "N/A";
    this->module_name = "N/A";

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
    this->t_last_uptime = 0;

    this->latency_avg_ = 0.0;
    this->latency_peak_ = 0.0;
    this->latency_total_ = 0;
}

EigenModule::~EigenModule(){

}

void EigenModule::add_downstream(uint8_t node_addr){
    std::lock_guard<std::mutex> lock(mutex);

    module_down_port port;
    port.addr_current = node_addr;
    port.addr_diff = node_addr;
    port.consistency_count = 0;
    downstream_list.push_back(port);
}

bool EigenModule::update_downstream(uint8_t ind, uint8_t node_addr){
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
        add_command(new EigenCommandUtility(address, EIGEN_UTIL_MODULE_PORTS));
        //firmware_utility(address, EIGEN_UTIL_MODULE_PORTS);
    }

    module_down_port downstream = downstream_list[ind];
    bool retval = false;
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
                retval = true;
            } else {
                //If we haven't seen the value enough times yet, keep checking to make sure it is correct
                add_command(new EigenCommandTopology(address));
            }
        }
        downstream_list[ind] = downstream;
    }

    return retval;
}

void EigenModule::clear_downstream(){
    std::lock_guard<std::mutex> lock(mutex);
    downstream_list.clear();
}

void EigenModule::set_downstream_name(uint8_t ind, std::string name){
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
std::string EigenModule::print_topology() const{
    std::lock_guard<std::mutex> lock(mutex);
    uint8_t s[128];
    uint8_t offset = 0;

    //Print the header
    offset += snprintf((char *)(s + offset), 128 - offset, " {\"id\":\"%02X\", \"type\":\"%02X\", \"orientation\":\"%02X\", \"children\":[",
                       (eigen_addr_t)address, (uint8_t)type, (uint8_t)orientation);

    //Print the body
    for(auto &item : downstream_list){
        offset += snprintf((char *)(s + offset), 128 - offset, "\"%02X\",", item.addr_current);
    }

    //Demarcate the end
    offset += snprintf((char *)(s + offset - 1), 128 - offset, "]}, ");

    return std::string((char *)s);
}

void EigenModule::update_UID(uint64_t UID_val){
    std::lock_guard<std::mutex> lock(mutex);

    if(UID == 0){ //If this is the first time we have seen a UID
        UID = UID_val;
    } else if(UID != UID_val){ //If we already have a UID and this one doesn't match, there must be a conflict.
        //add_command(this->address, CMD_UID_WR_ADDR, UID_); //Add a command to resolve this conflict
        add_command(new EigenCommandUIDWrite(UID, address));
    }
}
/*
void EigenModule::update_depth(uint8_t depth){
    node_depth_ = depth;
}

uint8_t EigenModule::node_depth() const{
    return node_depth_;
}

uint64_t EigenModule::UID() const{
    return UID_;
}*/

void EigenModule::update_type(uint8_t type_){
    std::lock_guard<std::mutex> lock(mutex);

    type = type_;
}

void EigenModule::update_orientation(uint8_t orientation_){
    std::lock_guard<std::mutex> lock(mutex);

    orientation = orientation_;
}

std::vector<module_down_port> EigenModule::downstream() const{
    std::lock_guard<std::mutex> lock(mutex);
    return downstream_list;
}

std::string EigenModule::print_mod_name() const{
    std::lock_guard<std::mutex> lock(mutex);
    uint8_t s[32];

    snprintf((char *)s, 32, "Module %02X", (eigen_addr_t)address);
    return std::string((char *)s);
}

std::string EigenModule::print_UID() const{
    std::lock_guard<std::mutex> lock(mutex);
    uint8_t s[32];

    snprintf((char *)s, 32, "u%016llX", (uint64_t)UID);
    return std::string((char *)s);
}

std::string EigenModule::print_type() const{
    std::lock_guard<std::mutex> lock(mutex);
    uint8_t s[32];

    snprintf((char *)s, 32, "Type: %d", (int)type);
    return std::string((char *)s);
}

uint8_t EigenModule::get_type() const{
    std::lock_guard<std::mutex> lock(mutex);
    return type;
}

uint8_t EigenModule::get_hardware_type() const{
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

std::string EigenModule::print_orientation() const{
    std::lock_guard<std::mutex> lock(mutex);
    uint8_t s[32];

    snprintf((char *)s, 32, "Orientation: %d", (int)orientation);
    return std::string((char *)s);
}

uint16_t EigenModule::get_encoder_status() const{
    std::lock_guard<std::mutex> lock(mutex);
    return this->encoder_status;
}

void EigenModule::set_encoder_status(uint16_t status){
    std::lock_guard<std::mutex> lock(mutex);
    this->encoder_status = status;
}

std::string EigenModule::print_encoder_status() const{
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

    return "ERROR";
}

/*  add_latency_measurement:
    A method that records packet latency so that it may be averaged over time.
    Calculates the moving average and stores it in a separate variable for later

    This method should only ever be called from a single thread
    There is no mutex protecting the deque. Thread safety comes from storage of
    final result in an std::atomic variable
*/
void EigenModule::add_latency_measurement(uint64_t latency){
    latencies_.push_back(latency);
    latency_total_ += latency;

    while(latencies_.size() > MAX_MOD_LATENCY_MEASUREMENT){
        latency_total_ -= latencies_.front();
        latencies_.pop_front();
    }

    latency_avg_ = ((double) latency_total_) / ((double) latencies_.size());

    if((double) latency > latency_peak_){
        latency_peak_ = (double)latency;
    }
}

double EigenModule::avg_latency() const{
    return latency_avg_;
}

double EigenModule::peak_latency() const{
    return latency_peak_;
}



