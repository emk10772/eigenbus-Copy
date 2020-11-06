#include "eigen_parameters.h"
//#include "eigen_comms.h"
#include <stdexcept>

template <class T>
void EigenParameterSet<T>::add_param(uint8_t id, T item, std::string name){
    std::lock_guard<std::mutex> lock(mutex);

    param_t param;
    param.name = "";
    param.dirty = false;
    param.value = T();

    if(id >= param_list.size()){
        //If the ID is past the end, resize to include it
        param_list.resize(id+1, param);
    }

    param.name = name;
    param.value = item;

    if(param_list[id].name == "")
        received_params++;

    //Update the param list
    param_list[id] = param;
}

template <class T>
void EigenParameterSet<T>::update_param(uint8_t param, T item){
    std::lock_guard<std::mutex> lock(mutex);

    if(param >= param_list.size()) return;
    param_list[param].value = item;
}

template <class T>
T EigenParameterSet<T>::read_param(uint8_t param){
    std::lock_guard<std::mutex> lock(mutex);

    if(param >= param_list.size()) return;
    return param_list[param].value;
}

template <class T>
void EigenParameterSet<T>::set_last_update(){
    std::lock_guard<std::mutex> lock(mutex);

    t_last_update = current_time_ms();
}

template <class T>
void EigenParameterSet<T>::set_expected_parameters(uint8_t num_parameters){
    std::lock_guard<std::mutex> lock(mutex);

    expected_num_params = num_parameters;
}

template <class T>
uint8_t EigenParameterSet<T>::parameters_left() const{
    std::lock_guard<std::mutex> lock(mutex);

    //TODO: Some sort of error fixing here. If this is wrong, we need to re check everything
    if(received_params > expected_num_params) return 0;

    return expected_num_params - received_params;
}

template <class T>
uint64_t EigenParameterSet<T>::d_t_last_update() const{
    return current_time_ms() - t_last_update;
}

template <class T>
std::string EigenParameterSet<T>::param_name(uint8_t id) const{
    std::lock_guard<std::mutex> lock(mutex);
    if(id >= param_list.size()) return "ERR";

    std::string retval = param_list[id].name;
    return retval;
}


EigenParameter::EigenParameter(std::string printed){
    try{
        if(printed.find_first_of('.') != std::string::npos){
            type_ = _DOUBLE;
            value_.double_ = stod(printed);
        } else {
            type_ = _UINT64;
            value_.uint64_ = stoull(printed, nullptr, EIGENBUS_BASE);
        }
    } catch(std::exception e){
        //If the conversion fails
        type_ = _DOUBLE;
        value_.double_ = nan("");
    }
}

EigenParameter::EigenParameter(uint8_t value){
    type_ = _UINT8;
    value_.uint8_ = value;
}

EigenParameter::EigenParameter(uint16_t value){
    type_ = _UINT16;
    value_.uint16_ = value;
}

EigenParameter::EigenParameter(uint32_t value){
    type_ = _UINT32;
    value_.uint32_ = value;
}

EigenParameter::EigenParameter(uint64_t value){
    type_ = _UINT64;
    value_.uint64_ = value;
}

EigenParameter::EigenParameter(float value){
    type_ = _FLOAT;
    value_.float_ = value;
}

EigenParameter::EigenParameter(double value){
    type_ = _DOUBLE;
    value_.double_ = value;
}

EigenParameter EigenParameter::from_type(uint8_t type){
    auto param = EigenParameter("");

    if(type > 0 && type <= PARAM_TYPE_MAX)
        param.type_ = type;

    return param;
}

EigenParameter::~EigenParameter(){

}


EigenParameter::eigen_param_t EigenParameter::value() const{
    return value_;
}

uint8_t EigenParameter::type() const{
    return type_;
}


std::string EigenParameter::print() const{
    return print_as(type_);
}

std::string EigenParameter::print_as(uint8_t type) const{
    switch(type){
    case _UINT8:
        return strprintf("%02X", value_.uint8_);
    case _UINT16:
        return strprintf("%04X", value_.uint16_);
    case _UINT32:
        return strprintf("%08X", value_.uint32_);
    case _UINT64:
        return strprintf("%016llX", value_.uint64_);
    case _FLOAT:
        return strprintf("%08.4f", value_.float_);
    case _DOUBLE:
        return strprintf("%08.4f", value_.double_);
    default:
        return "ERR";
    }
}

