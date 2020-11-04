#ifndef EIGEN_BOOTLOADER_H
#define EIGEN_BOOTLOADER_H

#include "eigen_utils.h"
#include "eigen_responses/eigen_response.h"
#include "cybootloaderutils\cybtldr_api2.h"
#include "cybootloaderutils\cybtldr_api.h"

class EigenBootloader{
public:
    EigenBootloader();
    ~EigenBootloader();

    void process_packet(EigenResponse *packet);
    bool active();

    int bootloader_open();
    int bootloader_close();
    int bootloader_read_data(uint8_t* buf, int len);
    int bootloader_write_data(uint8_t* buf, int len);
    void bootloader_update(uint8_t col, uint16_t row);
    void bootloader_init();
    std::string bootloader_print_error(int retval);

private:
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