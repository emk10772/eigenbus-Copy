#include "module.h"
#include "eigen_comms.h"
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
