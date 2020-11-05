#ifndef EIGEN_BOOTLOADER_H
#define EIGEN_BOOTLOADER_H

#include "eigen_utils.h"
#include "eigen_responses/eigen_response.h"
#include "eigen_commands/eigen_commands.h"
#include "cybootloaderutils/cybtldr_api2.h"
#include <deque>
//#include "cybootloaderutils/cybtldr_api.h"

#include <thread>

class EigenBootloader{
public:
    EigenBootloader();
    ~EigenBootloader();

    static void process_packet(EigenResponse *packet);
    static void process_command(EigenCommand *command);
    
    static int bootloader_open();
    static int bootloader_close();
    static int bootloader_read_data(uint8_t* buf, int len);
    static int bootloader_write_data(uint8_t* buf, int len);
    static void bootloader_update(uint8_t col, uint16_t row);
    static void bootloader_init();
    static std::string bootloader_print_error(int retval);
    static bool active();
    static bool finished();

    static EigenCommand *get_command();
    static void add_command(EigenCommand *);

private:
    static std::deque<EigenCommand *> cmd_list;
    static std::mutex cmd_mutex;

    static void run_operation(eigen_addr_t target_addr, uint8_t mode, std::string file);
    static std::thread bootload_thread;
    static bool EigenBootloader::bootloader_parse_packet(std::string data, uint8_t *buf, uint8_t len);

    static std::deque<std::string> bootloader_data;
    static CyBtldr_CommunicationsData comm_struct;
    static bool bootloader_active;
    static bool bootloader_ack;
    static uint8_t bootloader_target_addr;
    static uint8_t bootloader_mode;
    static std::string bootloader_file;
    static bool bootloader_finished;
    static std::string bootloader_last;
    static uint8_t bootloader_seq_num;
};

#endif