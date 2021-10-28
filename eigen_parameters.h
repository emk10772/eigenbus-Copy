/* eigen_parameters.h
 *
 * This file contains code related to keeping track of module parameters. It
 * provides a utility class for verifying lists of parameters.
 */

#ifndef EIGEN_PARAMETERS_H
#define EIGEN_PARAMETERS_H

#include "eigen_utils.h"

/* Param Types */
#define PARAM_TYPE_UINT8        (1)
#define PARAM_TYPE_UINT16       (2)
#define PARAM_TYPE_UINT32       (3)
#define PARAM_TYPE_FLOAT        (4)
#define PARAM_TYPE_UINT64       (5)
#define PARAM_TYPE_DOUBLE       (6)
#define PARAM_TYPE_MAX          (PARAM_TYPE_DOUBLE)

/* Mailbox Type Hints */
#define MAILBOX_RW_MASK         (0x0F)
#define MAILBOX_READ_ONLY       (0x01)
#define MAILBOX_READ_WRITE      (0x02)

#define MAILBOX_TYPE_MASK       (0xF0)
#define MAILBOX_INT             (0x10)
#define MAILBOX_DOUBLE          (0x20)
#define MAILBOX_STRING          (0x40)


/* Parameter Set Template
 *
 * This handles the initialization process of a parameter list. The steps for
 * initializing a list are as follows:
 *  1. Read the parameter list with a PARAM_READ command with param_id = 0
 *  2. Receive the number of parameters that we should expect
 *      -> Set the expected number of parameters in the parameter set
 *  3. Read each parameter based on the number of parameters expected
 *      -> Update the parameter set with each parameter we receive
 *  4. Verify the parameter list
 *      -> Make sure that we received as many parameters as expected
 *  5. If the parameter list is invalid, repeat 3-4
 */
template <class T>
class EigenParameterSet{
public:
    inline EigenParameterSet(){
        param_set_item_t param;
        param.name = "";
        param.dirty = false;
        param.value = nullptr;

        //Add a dummy entry at the start
        param_list.resize(1);
        param_list[0] = param;

        expected_num_params = 0xFF;
        received_params = 0;
        t_last_update = current_time_ms();
        request_in_progress = false;
    }

    inline ~EigenParameterSet(){
        for(param_set_item_t item : param_list){
            delete item.value;
        }
        param_list.clear();
    }

    /* void add(uint8_t id, T *item, std::string name)
     *
     * Add a new parameter to the set. Will pad the parameter list to fit
     * it in.
     */
    inline void add(uint8_t id, T *item, std::string name){
        std::lock_guard<std::mutex> lock(mutex);

        //Make an invalid param to pad the list with
        param_set_item_t param;
        param.name = "";
        param.dirty = false;
        param.value = nullptr;

        if(id >= param_list.size()){
            //If the ID is past the end, resize to include it
            param_list.resize(id+1, param);
        }

        //Set the struct name and value
        param.name = name;
        param.value = item;

        //If this is a new parameter mark that we received it
        if(param_list[id].name == "")
            received_params++;

        //Delete the old param if there was one here
        if(param_list[id].value)
            delete param_list[id].value;

        //Update the param list
        param_list[id] = param;
        t_last_update = current_time_ms();
    }

    inline void set_last_update(){
        std::lock_guard<std::mutex> lock(mutex);

        t_last_update = current_time_ms();
    }

    /* void set_expected_parameters(uint8_t num_parameters)
     *
     * Set the number of parameters that we expect to receive. Also marks
     * that we are beginning to receive parameters for the latest request
     */
    inline void set_expected_parameters(uint8_t num_parameters){
        std::lock_guard<std::mutex> lock(mutex);

        expected_num_params = num_parameters;
        t_last_update = current_time_ms();
        request_in_progress = false;
    }

    inline uint8_t parameters_left() const{
        std::lock_guard<std::mutex> lock(mutex);

        //TODO: Some sort of error fixing here. If this is wrong, we need to re check everything
        if(received_params > expected_num_params || expected_num_params == 0xFF) return 0;

        return expected_num_params - received_params;
    }

    inline uint8_t expected() const {
        return expected_num_params;
    }

    inline uint64_t d_t_last_update() const{
        return current_time_ms() - t_last_update;
    }

    inline void set_request_in_progress(bool value){
        request_in_progress = value;
    }

    /* bool update_required()
     *
     * An update is required in the following situation:
     *  There is an error in the parameter config (expected params == 0)
     *   OR there are parameters left AND...
     *      1. We haven't received new parameters in a while
     *      2. We aren't waiting on a recent request
     */
    inline bool update_required(){
        return (expected_num_params == 0xFF || parameters_left() > 0) &&
                d_t_last_update() > PACKET_TIMEOUT && !request_in_progress;
    }

    inline std::string name(uint8_t id) const{
        std::lock_guard<std::mutex> lock(mutex);
        if(id >= param_list.size()) return "ERR";

        std::string retval = param_list[id].name;
        return retval;
    }

    inline T *value(uint8_t param) const{
        std::lock_guard<std::mutex> lock(mutex);

        if(param >= param_list.size()) return nullptr;
        return param_list[param].value;
    }

    /* std::vector<const T *> list() const
     *
     * Generates a list of all of the parameters in this parameter set. This
     * is useful for generating EigenVariableGroups from EigenParameterSets.
     */
    inline std::vector<const T *> list() const{
        std::lock_guard<std::mutex> lock(mutex);

        std::vector<const T *> retval = {};
        for(auto &param : param_list){
            retval.emplace_back(param.value);
        }
        return retval;
    }

    /* bool valid() const
     *
     * Whether or not this parameter set is valid. A set is valid if:
     *  1. We have a nonzero number of parameters that we expect
     *      (AKA we have received a list param response)
     *  2. We are not waiting on any parameters to be filled in
     *      (We have successfully read every parameter)
     */
    inline bool valid() const{
        return expected_num_params != 0xFF && parameters_left() == 0;
    }

private:
    typedef struct{
        bool dirty;
        std::string name;
        T *value;
    } param_set_item_t;

    mutable std::mutex mutex;
    std::vector<param_set_item_t> param_list;
    uint64_t t_last_update;
    uint8_t expected_num_params;
    uint8_t received_params;
    bool request_in_progress;

};

#endif // EIGEN_PARAMETERS_H
