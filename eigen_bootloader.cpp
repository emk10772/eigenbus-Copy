#include "eigen_bootloader.h"
#include "eigen_comms.h"
#include "eigen_responses/eigen_response_bootloader.h"


EigenBootloader *EigenBootloader::instance = nullptr;

EigenBootloader *EigenBootloader::getInstance(){
    if(EigenBootloader::instance == nullptr){
        EigenBootloader::instance = new EigenBootloader();
    }

    return EigenBootloader::instance;
}

static unsigned short crc_table [256] = {
0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5,
0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b,
0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210,
0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c,
0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401,
0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b,
0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738,
0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5,
0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969,
0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc,
0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03,
0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6,
0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb,
0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1,
0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c,
0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2,
0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb,
0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447,
0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2,
0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827,
0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c,
0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0,
0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d,
0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07,
0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba,
0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

unsigned short CRCCCITT(unsigned char *data, size_t length, unsigned short seed, unsigned short final)
{

   size_t count;
   unsigned int crc = seed;
   unsigned int temp;

   for (count = 0; count < length; ++count)
   {
     temp = (*data++ ^ (crc >> 8)) & 0xff;
     crc = crc_table[temp] ^ (crc << 8);
   }

   return (unsigned short)(crc ^ final);

}

EigenBootloader::EigenBootloader(){
    comm_struct.MaxTransferSize = 52;
    comm_struct.OpenConnection = &bootloader_open;
    comm_struct.CloseConnection = &bootloader_close;
    comm_struct.ReadData = &bootloader_read_data;
    comm_struct.WriteData = &bootloader_write_data;

    bootloader_active = false;
    bootloader_ack = false;
    bootloader_mode = 0;
    bootload_thread = nullptr;
}

EigenBootloader::~EigenBootloader(){

}

#define PACKET_SIZE_LEN     (2) //"%02X"
#define PACKET_CRC_LEN      (4) //"%04X"
#define PACKET_SEQ_LEN      (2) //"%02X"
#define PACKET_HEADER_LEN   (PACKET_SIZE_LEN + 1 + PACKET_CRC_LEN + 1 + PACKET_SEQ_LEN + 1) //"%02X,%04X,%02X."
#define PROCESS_BUFFER_SIZE (256)

EigenCommand *EigenBootloader::get_command(){
    return cmd_list_.get();
}

void EigenBootloader::add_command(EigenCommand *command){
    cmd_list_.add(command);
}

EigenUpdate *EigenBootloader::get_update(){
    return update_list_.get();
}

void EigenBootloader::add_update(EigenUpdate *command){
    update_list_.add(command);
}

void EigenBootloader::run_operation(eigen_addr_t target_addr, uint8_t mode, std::string file){
    instance->add_command(new EigenCommandBootloader(target_addr, EigenCommandBootloader::BOOTLOADER_ACK));

    int retval = 0;
    if(mode == 1){
        retval = CyBtldr_Program(file.c_str(), NULL, (uint8_t) 3, &instance->comm_struct, &bootloader_update);
    } else if(mode == 2){
        retval = CyBtldr_Verify(file.c_str(), NULL, &instance->comm_struct, &bootloader_update);
    } else if(mode == 3) {
        retval = CyBtldr_Erase(file.c_str(), NULL, &instance->comm_struct, &bootloader_update);
    }

    //If successful, retval = 0. Mark as finished.
    if(!retval){
        instance->bootloader_finished = true;
    }

    add_update(new EigenUpdate(target_addr, EigenUpdate::MODULE_BTLDR_END, nullptr, retval, bootloader_print_error(retval)));
}

void EigenBootloader::process_packet(EigenResponse *packet){
    if(packet->message_type() != EigenResponse::EIGEN_BOOTLOADER) return;

    EigenResponseBootloader *btldr_packet = static_cast<EigenResponseBootloader *>(packet);
    switch(btldr_packet->btldr_action()){
        case EigenResponseBootloader::BOOTLOADER_ACK:
            if(bootloader_active == true && bootloader_target_addr == packet->address()){
                if(!bootloader_ack) {
                    bootloader_ack = true;

                    //TODO: Something more responsible here
                    //Could cause memory leaks and other errors if the programming gets stuck
                    if(bootload_thread != nullptr){
                        //delete bootload_thread;
                        bootload_thread = nullptr;
                    }
                    bootload_thread = new std::thread(&EigenBootloader::run_operation,
                                                  this, bootloader_target_addr, bootloader_mode, bootloader_file);
                } else {
                    add_command(new EigenCommandBootloader(bootloader_target_addr, EigenCommandBootloader::BOOTLOADER_ACK));
                }
            }
            break;
        case EigenResponseBootloader::BOOTLOADER_READ:
            bootloader_data.push_back(btldr_packet->data());
            break;
        case EigenResponseBootloader::BOOTLOADER_RESEND:
            add_command(new EigenCommandUser(bootloader_target_addr, bootloader_last));
            break;
        default:
            break;
    }
    
}

void EigenBootloader::process_command(EigenCommand *command){
    if(command->command_type() != EigenCommand::EIGEN_CMD_BOOTLOADER) return;

    EigenCommandBootloader *btldr_command = static_cast<EigenCommandBootloader *>(command);
    switch(btldr_command->action()){
        case EigenCommandBootloader::BOOTLOADER_START:
            bootloader_ack = false;
            bootloader_target_addr = btldr_command->address();
            bootloader_active = true;
            bootloader_mode = btldr_command->mode();
            bootloader_file = btldr_command->msg();
            break;
        case EigenCommandBootloader::BOOTLOADER_ACK:
            break;
        case EigenCommandBootloader::BOOTLOADER_RESEND:
            break;
        default:
            break;
    }
}

/* Bootloader Interface */
#define MAX_PACKET_CHARS    60
#define OUT_BUF_SIZE        128

int EigenBootloader::bootloader_open(void){
    instance->bootloader_finished = false;
    instance->bootloader_seq_num = 0;

    if(instance->bootloader_ack) {
        return CYRET_SUCCESS;
    }

    return CYRET_ABORT;
}

int EigenBootloader::bootloader_close(void){
    instance->bootloader_active = false;
    instance->bootloader_target_addr = 0xFF;
    instance->bootloader_ack = false;

    return CYRET_SUCCESS;
}

bool EigenBootloader::bootloader_parse_packet(std::string data, uint8_t *buf, uint8_t len){
    uint8_t packet_len = data.size() - PACKET_HEADER_LEN;
    if(data.size() < PACKET_HEADER_LEN || packet_len % 2){
        add_command(new EigenCommandBootloader(bootloader_target_addr, "LEN"));
        return false;
    }

    uint8_t ind = 0;
    unsigned int c = 0;
    char aux_buffer[PROCESS_BUFFER_SIZE];
    uint8_t valid = 1;

    // Process the header
    char *ptr = (char *)data.c_str();
    uint8_t num_chars = strtol(ptr, &ptr, 16);
    if(*ptr != ',') valid = 0;
    ++ptr;
    uint16_t crc_packet = strtol(ptr, &ptr, 16);
    if(*ptr != ',') valid = 0;
    ++ptr;
    uint8_t sequence_num = strtol(ptr, &ptr, 16);
    if(*ptr != ',') valid = 0;
    ++ptr;

    /* Move chars into the buffer until we run out of space or data */
    while(valid && 2*ind < packet_len){
        if(!isxdigit((uint8_t)(data.c_str()[2*ind + PACKET_HEADER_LEN])) ||
                !isxdigit((uint8_t)(data.c_str()[2*ind + PACKET_HEADER_LEN + 1]))){
            valid = 0;
            break;
        }

        int retval = sscanf((char *)data.c_str() + 2*ind + PACKET_HEADER_LEN, "%02x", &c);

        if(retval != 1){
            valid = 0;
            break;
        }

        aux_buffer[ind] = (uint8_t) c;
        ind += 1;
    }

    uint16_t crc_calc = CRCCCITT((uint8_t *)aux_buffer, ind, 0xFFFF, 0);

    if(!valid){
        add_command(new EigenCommandBootloader(bootloader_target_addr, "INVLD"));
        return false;
    } else if((2*ind) != packet_len){
        add_command(new EigenCommandBootloader(bootloader_target_addr, "LEN"));
        return false;
    } else if(ind != num_chars) {
        add_command(new EigenCommandBootloader(bootloader_target_addr, "CHAR"));
        return false;
    } else if (crc_calc != crc_packet) {
        add_command(new EigenCommandBootloader(bootloader_target_addr, "CRC"));
        return false;
    } else if (sequence_num != bootloader_seq_num){
        add_command(new EigenCommandUser(bootloader_target_addr, bootloader_last));
        return false;
    } else {
        //Put all of the data in the buffer
        for(uint8_t i = 0; i < ind; i++){
            buf[i] = aux_buffer[i];
        }
        return true;
    }
}

int EigenBootloader::bootloader_read_data(uint8_t* buf, int len){
    uint64_t t_start = current_time_ms();
    uint64_t t_last = t_start;
    uint8_t n_chars = 0;
    uint8_t first_request_n = 0;
    bool success = false;

    while(!success && current_time_ms() - t_start < 500000){
        if(instance->bootloader_data.size() > 0){
            std::string data = instance->bootloader_data.front();
            instance->bootloader_data.pop_front();

            success = instance->bootloader_parse_packet(data, buf, len);
            t_last = current_time_ms();
        } else if (current_time_ms() - t_last > 250) {
            instance->add_command(new EigenCommandBootloader(instance->bootloader_target_addr, "TIME"));
            t_last = current_time_ms();
        }
    }
    
    if(success) 
        return CYRET_SUCCESS;
    else
        return CYRET_ERR_UNK;
    
}

int EigenBootloader::bootloader_write_data(uint8_t* buf, int len){
    char out_buf[OUT_BUF_SIZE];

    uint16_t crc = CRCCCITT(buf, len, 0xFFFF, 0);

    uint16_t packet_ind = 0;
    while(packet_ind < len){
        //Print the header
        uint8_t ct = 0;
        uint8_t ind = snprintf(out_buf, OUT_BUF_SIZE, "%02X~d%02X,%04X,%02X,",
                               instance->bootloader_target_addr, len, crc, instance->bootloader_seq_num);

        //Print the data characters
        while(ind < OUT_BUF_SIZE && ct < MAX_PACKET_CHARS && packet_ind < len){
            ind += snprintf(out_buf + ind, OUT_BUF_SIZE - ind, "%02X", buf[packet_ind]);
            ct++;
            packet_ind++;
        }

        uint8_t crc_2 = crc_8_ccitt(out_buf, ind);

        instance->bootloader_last = std::string(out_buf);
        instance->add_command(new EigenCommandUser(instance->bootloader_target_addr, out_buf));
    }

    //Increase the sequence counter for each packet sent
    instance->bootloader_seq_num++;
    //add_packet(bootloader_target_addr, out_buf, "~r");
    return CYRET_SUCCESS;
}

void EigenBootloader::bootloader_update(uint8_t col, uint16_t row){
    instance->add_update(new EigenUpdate(instance->bootloader_target_addr,
                                         EigenUpdate::MODULE_BTLDR_PROGRESS, nullptr, row));
}

bool EigenBootloader::active(){
    return bootloader_active;
}

bool EigenBootloader::finished(){
    if(bootloader_finished){
        bootloader_finished = false;
        return true;
    }
    return false;
}

std::string EigenBootloader::bootloader_print_error(int retval){
    std::string printed;
    if(retval & CYRET_ERR_COMM_MASK){
        printed = "Communications Error: ";
        int masked = retval & 0xFF;

        if(masked == CYRET_SUCCESS){
            printed.append("Completed Successfully");
        } else if(masked == CYRET_ERR_FILE){
            printed.append("File is not accessible");
        } else if(masked == CYRET_ERR_EOF){
            printed.append("Reached the end of the file");
        } else if(masked == CYRET_ERR_LENGTH){
            printed.append("The amount of data available is outside the expected range");
        } else if(masked == CYRET_ERR_DATA){
            printed.append("The data is not of the proper form");
        } else if(masked == CYRET_ERR_CMD){
            printed.append("The command is not recognized");
        } else if(masked == CYRET_ERR_DEVICE){
            printed.append("The expected device does not match the detected device");
        } else if(masked == CYRET_ERR_VERSION){
            printed.append("The bootloader version detected is not supported");
        } else if(masked == CYRET_ERR_CHECKSUM){
            printed.append("The checksum does not match the expected value");
        } else if(masked == CYRET_ERR_ARRAY){
            printed.append("The flash array is not valid");
        } else if(masked == CYRET_ERR_ROW){
            printed.append("The flash row is not valid");
        } else if(masked == CYRET_ERR_BTLDR){
            printed.append("The bootloader is not ready to process data");
        } else if(masked == CYRET_ERR_ACTIVE){
            printed.append("The application is currently marked as active");
        } else if(masked == CYRET_ERR_UNK){
            printed.append("An unknown error occurred");
        } else if(masked == CYRET_ABORT){
            printed.append("The operation was aborted");
        }
    } else if(retval & CYRET_ERR_BTLDR_MASK) {
        printed = "Bootloader Error: ";
        int masked = retval & 0xFF;

        if(masked == CYBTLDR_STAT_SUCCESS){
            printed.append("Completed Successfully");
        } else if(masked == CYBTLDR_STAT_ERR_KEY){
            printed.append("The provided key does not match the expected value");
        } else if(masked == CYBTLDR_STAT_ERR_VERIFY){
            printed.append("The verification of flash failed");
        } else if(masked == CYBTLDR_STAT_ERR_LENGTH){
            printed.append("The amount of data available is outside the expected range");
        } else if(masked == CYBTLDR_STAT_ERR_DATA){
            printed.append("The data is not of the proper form");
        } else if(masked == CYBTLDR_STAT_ERR_CMD){
            printed.append("The command is not recognized");
        } else if(masked == CYBTLDR_STAT_ERR_DEVICE){
            printed.append("The expected device does not match the detected device");
        } else if(masked == CYBTLDR_STAT_ERR_VERSION){
            printed.append("The bootloader version detected is not supported");
        } else if(masked == CYBTLDR_STAT_ERR_CHECKSUM){
            printed.append("The checksum does not match the expected value");
        } else if(masked == CYBTLDR_STAT_ERR_ARRAY){
            printed.append("The flash array is not valid");
        } else if(masked == CYBTLDR_STAT_ERR_ROW){
            printed.append("The flash row is not valid");
        } else if(masked == CYBTLDR_STAT_ERR_PROTECT){
            printed.append("The bootloader is not ready to process data");
        } else if(masked == CYBTLDR_STAT_ERR_APP){
            printed.append("The application is currently marked as active");
        } else if(masked == CYRET_ERR_UNK){
            printed.append("An unknown error occurred");
        }
    } else {
        printed.append("Programmed Successfully");
    }


    return printed;
}
