#ifndef _MODELS_PORT_H_

#define _MODELS_PORT_H_ 1

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/dir.h>

#include <curl/curl.h>

#define _MODEL_T_ 1
#define _MODEL_5_ 2
#define _MODEL_4_ 3

#define PROG_VERSION "5.0"

/*
Versiomn history.
4.0 - first raspberry PI oriented version. All stdout output is supressed. Will work on other Linux systems.
4.1 - Fixed the case when upon start Cimel tries to upload the entore memory, but after reaching the bottom reverses back to the top. The variable "previous_time" is introduced to fix the issue.

5.0 - joined V5 and T in a common software 

*/

typedef struct
{
    int status;
    size_t text_size;
    char *text;
} UPLOAD_RESPONSE;

typedef struct
{
    time_t aeronet_time;
    char pc_time_string[20];
    int aeronet_time_real, seconds_shift, status, good_clock;
} AERO_EXCHANGE;

typedef struct
{
    unsigned char *buffer, idbyte;
    int record_size;
    time_t record_time;
} RECORD_BUFFER;

typedef struct
{
    RECORD_BUFFER header, *records;
    int cimel_number,
        num_records, allocated_records, if_header;
    char eprom[20], file_name[300], real_file_name[500];
    UPLOAD_RESPONSE up_res;
} CIMEL_BUFFER;

typedef struct
{
    int fd, packet_timeout;

    char port_name[20], *dir_name, hostname[40], program_version[10];
    int if_open_port, time_interval;
    size_t length_ret, length_data;

    time_t cimel_time, last_time_T, last_time_5, check_time, previous_time;

    unsigned char packet_received[4000], packet_send[20], payload_received[2000], *buf, message_number;

    char eprom_T[20], eprom_5[20];

    unsigned char buffer[2000], *begin, *end;
    int if_flag, header_flag, time_header_flag, empty_event_count, if_T_port_already_open,
        time_correction_flag, time_count, cimel_number_T, cimel_number_5, cimel_model;
} MY_COM_PORT;

// common V4, V5 and T functions

void output_message_to_log(char *log_file, char *message_text);

int define_usb_com_port(char *usb_port, char *log_file);

int connect_hologram_model_and_reset_if_error(char *usb_reset_command, int *reset_counter, char *log_file);

size_t handle_aeronet_time_internally(unsigned char *buffer, size_t size, size_t nmemb, AERO_EXCHANGE *ptr);
time_t receive_aeronet_time(AERO_EXCHANGE *ptr, char *log_file);

void close_my_port(MY_COM_PORT *mcport);
int open_my_com_port(MY_COM_PORT *mcport, int cimel_model, char *log_file);
int reading_single_port_with_timeout(MY_COM_PORT *mcport);

void init_cimel_buffer(CIMEL_BUFFER *ptr);
void free_cimel_buffer(CIMEL_BUFFER *ptr);

static size_t analyze_aeronet_response(void *data, size_t size, size_t nmemb, void *userp);
int libcurl_upload_cimel_buffer_to_https(CIMEL_BUFFER *ptr, char *log_file);

int upload_daily_connection_log_to_ftp(char *log_name, char *username);

void copy_records(RECORD_BUFFER *ptr1, RECORD_BUFFER *ptr2);
void combine_cimel_buffers(CIMEL_BUFFER *main_buffer, CIMEL_BUFFER *new_buffer);

//  Specific T functions

size_t T_CRC16_Compute(unsigned char *pBuffer, size_t length);
size_t T_CRC16_Compute_with_number(unsigned char number, unsigned char *pBuffer, size_t length);
unsigned short T_CRC16_Compute_with_one_number(unsigned char number, unsigned char command_byte);
void T_form_packet_to_send(unsigned char number, unsigned char command_byte, unsigned char *packet_send);
unsigned char T_check_received_packet(unsigned char *packet_received, size_t length);
unsigned short T_convert_hex_ascii_coded_to_byte(unsigned char *code, unsigned char *res, int length);

int T_receive_packet_from_port(MY_COM_PORT *mcport, unsigned char command_byte);

int T_receive_cimel_time_from_port(MY_COM_PORT *mcport);
time_t T_convert_bytes_to_time_t(unsigned char *buf);

unsigned char T_retrieve_new_record(unsigned char *buf, size_t record_size, RECORD_BUFFER *ptr);

int T_receive_header_from_port(MY_COM_PORT *mcport, CIMEL_BUFFER *ptr, char *log_file);
int T_retrieve_k8_buffer_data_only(MY_COM_PORT *mcport, CIMEL_BUFFER *ptr, int max_num, char *log_file);

int T_save_k8_buffer_on_disk(char *alt_name, CIMEL_BUFFER *ptr);
time_t T_read_k8_buffer_from_disk(char *alt_name, CIMEL_BUFFER *ptr);

// Specific V4 and V5 functions

void V5_put_sys_time_to_buffer(time_t *sys_time, unsigned char *buffer);
time_t V5_get_sys_time_from_buffer(unsigned char *buffer);

unsigned char V5_convert_char_to_BYTE(unsigned char ch);
unsigned char V5_get_decimall(unsigned char ch);
unsigned char V5_convert_block(unsigned char *buffer, unsigned char *result, int num);

time_t V5_get_cimel_time(unsigned char *buffer, int length);

int V5_check_buffer_for_checksum(MY_COM_PORT *mcport);
void V5_init_port_receiption(MY_COM_PORT *mcport);
void V5_wait_for_new_packet(MY_COM_PORT *mcport);
void V5_send_request_to_cimel(MY_COM_PORT *mcport, char *request, int num_bytes);

int V5_check_if_packet_completed(MY_COM_PORT *mcport, AERO_EXCHANGE *aerex);
int V5_main_loop_cycle(MY_COM_PORT *mcport, AERO_EXCHANGE *aerex, CIMEL_BUFFER *k7_buffer, char *log_file);

int V5_save_k7_buffer_on_disk(char *dir_name, CIMEL_BUFFER *ptr);
time_t V5_read_k7_buffer_from_disk(char *dir_name, CIMEL_BUFFER *ptr);

#endif
