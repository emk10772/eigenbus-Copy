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
    void process_packet(EigenResponse *packet);
    void process_command(EigenCommand *command);
    
    static int bootloader_open();
    static int bootloader_close();
    static int bootloader_read_data(uint8_t* buf, int len);
    static int bootloader_write_data(uint8_t* buf, int len);
    static void bootloader_update(uint8_t col, uint16_t row);
    static void bootloader_init();
    std::string bootloader_print_error(int retval);
    bool active();
    bool finished();

    EigenCommand *get_command();
    void add_command(EigenCommand *);

    static EigenBootloader *getInstance();

private:
    //Singleton design pattern
    static EigenBootloader *instance;
    explicit EigenBootloader();
    ~EigenBootloader();

    std::deque<EigenCommand *> cmd_list;
    std::mutex cmd_mutex;

    void run_operation(eigen_addr_t target_addr, uint8_t mode, std::string file);
    std::thread bootload_thread;
    bool bootloader_parse_packet(std::string data, uint8_t *buf, uint8_t len);

    std::deque<std::string> bootloader_data;
    CyBtldr_CommunicationsData comm_struct;
    bool bootloader_active;
    bool bootloader_ack;
    uint8_t bootloader_target_addr;
    uint8_t bootloader_mode;
    std::string bootloader_file;
    bool bootloader_finished;
    std::string bootloader_last;
    uint8_t bootloader_seq_num;
};

#endif