#ifndef EIGEN_PACKET_POLL_H
#define EIGEN_PACKET_POLL_H

#include "eigen_utils.h"
#include "eigen_commands/eigen_command.h"
#include <map>
#include <deque>

class EigenPacketPoll{
public:
    typedef struct{
        EigenCommand *command;
        bool enabled;
        uint64_t period_ms;
        uint64_t t_last;
    } poll_t;

    EigenPacketPoll();
    ~EigenPacketPoll();

    std::string add_command(EigenCommand *command, uint64_t period_ms, bool enabled = true);
    void set_enable(std::string key, bool enabled);
    void remove_command(std::string key);

    void service_poll();
    EigenCommand *get_command();


private:
    std::map<std::string, poll_t *> poll_map_;
    std::mutex poll_mutex_;

    EigenQueue<EigenCommand> commands;
};

#endif
