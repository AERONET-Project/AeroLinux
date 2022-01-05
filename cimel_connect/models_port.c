#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

#include <curl/curl.h>

#include "models_port.h"

// common V4, V5 and T functions

int define_usb_com_port(char *usb_port)
{
    char buffer[100], full_path[200], *ch;
    DIR *dfd;
    struct direct *dp;
    size_t size_dev;
    int status = 0;

    dfd = opendir("/dev/serial/by-id");
    if (dfd == NULL)
        return 0;

    while ((dp = readdir(dfd)) != NULL)
    {

        if (dp->d_type == DT_LNK)
        {
            if (strstr(dp->d_name, "FTDI"))
            {

                sprintf(full_path, "/dev/serial/by-id/%s", dp->d_name);

                size_dev = readlink(full_path, buffer, 90);
                if (size_dev != -1)
                {
                    buffer[size_dev] = '\0';

                    ch = strstr(buffer, "ttyUSB");
                    if (ch != NULL)
                    {
                        sprintf(usb_port, "/dev/%s", ch);
                        printf("Found USB serial port at %s\n", usb_port);
                        status = 1;
                    }
                }
            }
        }
    }
    closedir(dfd);

    return status;
}

int define_usb_reset_command (FILE *in, char *usb_reset_command)
{
char *ch1, *ch2, lsusb_string[256];

while (fgets(lsusb_string, 255, in))
{
    ch1 = strstr (lsusb_string, ": ID ");
    if (ch1 != NULL)
    {
      ch1 += 5;
      ch1[9] = '\0';
      if (!strncmp(ch1 + 10,"Terminus", 8))
      {
          sprintf (usb_reset_command,"sudo /usr/bin/usbreset %s", ch1);
          return 1;
      }
    }
}

return  0;
}



int connect_hologram_model_and_reset_if_error (char *usb_reset_command, int *reset_counter)
{
time_t pc_time, stop_time;
FILE *rd;
char buffer[101];
printf("Will activate modem\n");
pc_time = time(NULL);
rd = popen("sudo hologram network connect 2>&1","r"); // redirect error output to stdout

if (rd == NULL) 
{
printf ("Unable to open process\n"); // if popen does not work, terminate program
return 0;
}

fgets (buffer, 100, rd);
pclose(rd);

if (!strstr (buffer,"ERROR")) 
{
    stop_time = time(NULL);
    printf ("%s\nConnected in %d seconds\n",buffer, stop_time - pc_time);
    return 1;  // Normal connection.
}

printf ("Found error in connection\n%s\nWill reset USB hub [%s]\n", buffer,usb_reset_command);

system (usb_reset_command);
*reset_counter = *reset_counter + 1;

rd = popen("sudo hologram network connect 2>&1","r"); // redirect error output to stdout

if (rd == NULL) 
{
printf ("Unable to open process\n"); // if popen does not work, terminate program
return 0;
}

fgets (buffer, 100, rd);
pclose(rd);


if (!strstr (buffer,"ERROR")) 
{
    stop_time = time(NULL);
    printf ("%s\nConnected in %d seconds\n",buffer, stop_time - pc_time);
    return 1;  // Normal connection.
}


printf ("Found error in connection even after reset\n%s\nWill reboot\n", buffer);
system ("/home/$USER/USB_handler.sh");

return 0;

}






size_t handle_aeronet_time_internally(unsigned char *buffer, size_t size, size_t nmemb, AERO_EXCHANGE *ptr)
{

    int i;

    time_t aertime;
    size_t strsize = nmemb * size;
    char time_string[70], *small_string, *ch;

    if (strsize > 60)
        return 0;

    for (i = 0; i < strsize; i++)
        time_string[i] = buffer[i];
    time_string[i] = '\0';

    if (strncmp(time_string, "AERONET Time,", 13))
        return 0;

    small_string = time_string + 13;

    ch = small_string;
    while ((*ch != ',') && (*ch != '\0'))
        ch++;

    if (*ch != ',')
        return 0;

    *ch++ = '\0';

    if (!strncmp(ch, "PC Time,", 8))
        if (!strcmp(ch + 8, ptr->pc_time_string))
            ptr->aeronet_time_real = 1;

    aertime = atol(small_string);

    ptr->aeronet_time = aertime;
    return nmemb * size;
}

time_t receive_aeronet_time(AERO_EXCHANGE *ptr)
{
    CURL *curl;
    CURLcode res;
    time_t cr_time;
    struct tm mtim;

    time_t pi_time;

    char url_ref[100];

    ptr->status = ptr->good_clock = 0;

    cr_time = time(NULL);
    gmtime_r(&cr_time, &mtim);
    sprintf(ptr->pc_time_string, "%02d%02d%02d%02d%02d.000",
            mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour, mtim.tm_min, mtim.tm_sec);

    sprintf(url_ref, "https://aeronet.gsfc.nasa.gov/cgi-bin/aeronet_time_new?pc_time=%s", ptr->pc_time_string);

    ptr->aeronet_time_real = 0;

    curl = curl_easy_init();

    if (!curl)
    {
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url_ref);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_aeronet_time_internally);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, ptr);

    res = curl_easy_perform(curl);

    pi_time = time(NULL);
    printf("System time is %s", ctime(&pi_time));

    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {

        printf("Code error = %d\nAeronet time cannot be acquired\n", res);

        return 0;
    }

    if (ptr->aeronet_time_real)
    {
        ptr->seconds_shift = ptr->aeronet_time - pi_time;
        ptr->status = 1;
        printf("Aeronet time acquired : %sTime shift = %d seconds",
               ctime(&ptr->aeronet_time), ptr->seconds_shift);

        if (abs(ptr->seconds_shift) < 3)
        {
            ptr->good_clock = 1;
            printf(" System clock is good");
        }
        else
        {
            printf(" System clock will not be used to correct Cimel clock");
        }
    }
    else
    {
        printf("Aeronet time cannot be acquired at the moment, may need to work offline");
    }

    printf("\n");

    return ptr->aeronet_time;
}

void close_my_port(MY_COM_PORT *mcport)
{

    if (!mcport->if_open_port)
        return;

    close(mcport->fd);
    mcport->if_open_port = 0;
    mcport->if_T_port_already_open = 0;
}

int open_my_com_port(MY_COM_PORT *mcport, int cimel_model)
{

    struct termios options;

    if (mcport->if_open_port)
        return 1;

    mcport->fd = open(mcport->port_name, O_RDWR | O_NOCTTY | O_NDELAY);

    if (mcport->fd < 1)
        return 0;

    fcntl(mcport->fd, F_SETFL, 0);

    tcflush(mcport->fd, TCIOFLUSH);

    tcgetattr(mcport->fd, &options);

    if (cimel_model == _MODEL_T_)
        cfsetspeed(&options, B9600);
    else
        cfsetspeed(&options, B1200);

    mcport->cimel_model = cimel_model;

    cfmakeraw(&options);

    tcsetattr(mcport->fd, TCSANOW, &options);

    mcport->if_open_port = 1;
    mcport->if_T_port_already_open = (cimel_model == _MODEL_T_);

    mcport->message_number = 0x21;
    mcport->check_time = time(NULL);

    printf("Port opened with %d baud\n", 1200 + (2 - cimel_model) * 8400);

    return 1;
}

/*
reading_single_port_with_timeout returns:
-3 : error (port is closed) calling function should abort 
-2, 0 : no response from port during packet_timeout seconds.

1 - proper reading to mcport->buf[-1] (buf is incremented)

*/

int reading_single_port_with_timeout(MY_COM_PORT *mcport)
{

    fd_set rfds;
    struct timeval timeout;
    int retval, read_bytes;
    unsigned char byte;

    FD_ZERO(&rfds);
    FD_SET(mcport->fd, &rfds);

    timeout.tv_sec = mcport->packet_timeout;
    timeout.tv_usec = 0;
    retval = select(mcport->fd + 1, &rfds, NULL, NULL, &timeout);

    if (retval == -1)
    {
        close_my_port(mcport);
        return -3;
    }

    if (retval == 0)
        return -2;

    if (FD_ISSET(mcport->fd, &rfds))
    {

        read_bytes = read(mcport->fd, &byte, 1);

        if (read_bytes == 1)
        {

            if (mcport->cimel_model != _MODEL_T_)
            {
                if ((mcport->begin == NULL) && (byte == 2))
                    mcport->begin = mcport->buf;
                if ((mcport->end == NULL) && (byte == 23))
                    mcport->end = mcport->buf;
            }
            *mcport->buf++ = byte;
            return 1;
        }
    }

    return 0;
}

void init_cimel_buffer(CIMEL_BUFFER *ptr)
{
    ptr->num_records = ptr->allocated_records = ptr->if_header = 0;
    ptr->records = NULL;
    ptr->up_res.text_size = 0;
}

void free_cimel_buffer(CIMEL_BUFFER *ptr)
{
    int i;
    if (ptr->num_records)
    {
        for (i = 0; i < ptr->num_records; i++)
            free(ptr->records[i].buffer);
    }

    if (ptr->allocated_records)
    {
        free(ptr->records);
        free(ptr->header.buffer);
    }

    if (ptr->up_res.text_size)
        free(ptr->up_res.text);

    init_cimel_buffer(ptr);
}

static size_t analyze_aeronet_response(void *data, size_t size, size_t nmemb, void *userp)
{

    size_t realsize = size * nmemb;

    UPLOAD_RESPONSE *result = (UPLOAD_RESPONSE *)userp;
    result->text = (char *)malloc(realsize + 1);

    memcpy(result->text, data, realsize);
    result->text[realsize] = '\0';
    result->text_size = realsize;

    if (strstr(result->text, "file provided has been queued for processing"))
        result->status = 1;
    else
        result->status = 0;

    return result->text_size;
}

int libcurl_upload_cimel_buffer_to_https(CIMEL_BUFFER *ptr)
{
    CURL *curl;
    CURLcode res;

    curl_mime *multipart;
    curl_mimepart *part;

    unsigned char *buffer, *buf;
    size_t buf_size;
    int i;

    if (!ptr->if_header)
        return 0;

    curl = curl_easy_init();

    if (!curl)
        return 0;

    buf_size = ptr->header.record_size;

    for (i = 0; i < ptr->num_records; i++)
        buf_size += ptr->records[i].record_size;

    buf = buffer = (unsigned char *)malloc(buf_size);

    memcpy(buf, ptr->header.buffer, ptr->header.record_size);
    buf += ptr->header.record_size;

    for (i = ptr->num_records - 1; i >= 0; i--)
    {
        memcpy(buf, ptr->records[i].buffer, ptr->records[i].record_size);
        buf += ptr->records[i].record_size;
    }

    multipart = curl_mime_init(curl);

    part = curl_mime_addpart(multipart);
    curl_mime_name(part, "uploaded_file");
    curl_mime_data(part, ptr->file_name, CURL_ZERO_TERMINATED);

    part = curl_mime_addpart(multipart);

    curl_mime_name(part, "uploaded_file");
    curl_mime_data(part, buffer, buf_size);
    curl_mime_filename(part, ptr->file_name);
    curl_mime_type(part, "application/octet-stream");

    curl_easy_setopt(curl, CURLOPT_URL, "https://aeronet.gsfc.nasa.gov/cgi-bin/webfile_trans_auto");

    curl_easy_setopt(curl, CURLOPT_MIMEPOST, multipart);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, analyze_aeronet_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&ptr->up_res);

    res = curl_easy_perform(curl);
    curl_mime_free(multipart);

    curl_easy_cleanup(curl);

    free(buffer);

    if (res != CURLE_OK)
    {
        printf("Code error = %d  Upload unsuccesful\n", res);

        return 0;
    }

    printf("Upload succesful\n");

    return 1;
}

void copy_records(RECORD_BUFFER *ptr1, RECORD_BUFFER *ptr2)
{

    ptr1->record_size = ptr2->record_size;

    ptr1->idbyte = ptr2->idbyte;
    ptr1->record_time = ptr2->record_time;

    ptr1->buffer = (unsigned char *)malloc(ptr1->record_size);
    memcpy(ptr1->buffer, ptr2->buffer, ptr1->record_size);
}

void combine_cimel_buffers(CIMEL_BUFFER *main_buffer, CIMEL_BUFFER *new_buffer)
{
    int i, new_num, ii;
    RECORD_BUFFER *new_records;

    if (!new_buffer->if_header)
        return;

    if (!main_buffer->if_header)
    {
        copy_records(&main_buffer->header, &new_buffer->header);
        main_buffer->if_header = 1;
        main_buffer->cimel_number = new_buffer->cimel_number;
        strcpy(main_buffer->eprom, new_buffer->eprom);
        if (new_buffer->num_records)
        {
            main_buffer->allocated_records = main_buffer->num_records = new_buffer->num_records;
            main_buffer->records = (RECORD_BUFFER *)malloc(sizeof(RECORD_BUFFER) * main_buffer->allocated_records);
            for (i = 0; i < new_buffer->num_records; i++)
                copy_records(main_buffer->records + i, new_buffer->records + i);
        }
    }
    else if (new_buffer->num_records)
    {
        new_num = main_buffer->allocated_records + new_buffer->num_records;
        new_records = (RECORD_BUFFER *)malloc(sizeof(RECORD_BUFFER) * new_num);

        for (i = 0; i < new_buffer->num_records; i++)
            copy_records(new_records + i, new_buffer->records + i);
        if (main_buffer->num_records)
            for (i = 0; i < main_buffer->num_records; i++)
                copy_records(new_records + i + new_buffer->num_records, main_buffer->records + i);

        main_buffer->allocated_records = main_buffer->num_records = new_num;

        if (main_buffer->allocated_records)
            free(main_buffer->records);

        main_buffer->records = new_records;
    }
}

//  Specific T functions

size_t T_CRC16_Compute(unsigned char *pBuffer, size_t length)
{
    unsigned char i;
    int bs;
    size_t crc = 0;
    while (length--)
    {
        crc ^= *pBuffer++;
        for (i = 0; i < 8; i++)
        {
            bs = crc & 1;
            crc >>= 1;
            if (bs)
            {
                crc ^= 0xA001;
            }
        }
    }
    return crc;
}

size_t T_CRC16_Compute_with_number(unsigned char number, unsigned char *pBuffer, size_t length)
{
    unsigned char i;
    int bs;
    size_t crc = 0;

    crc ^= number;
    for (i = 0; i < 8; i++)
    {
        bs = crc & 1;
        crc >>= 1;
        if (bs)
        {
            crc ^= 0xA001;
        }
    }

    while (length--)
    {
        crc ^= *pBuffer++;
        for (i = 0; i < 8; i++)
        {
            bs = crc & 1;
            crc >>= 1;
            if (bs)
            {
                crc ^= 0xA001;
            }
        }
    }
    return crc;
}

unsigned short T_CRC16_Compute_with_one_number(unsigned char number, unsigned char command_byte)
{
    unsigned char i;
    int bs;
    unsigned short crc = 0;

    crc ^= number;
    for (i = 0; i < 8; i++)
    {
        bs = crc & 1;
        crc >>= 1;
        if (bs)
        {
            crc ^= 0xA001;
        }
    }

    crc ^= command_byte;
    for (i = 0; i < 8; i++)
    {
        bs = crc & 1;
        crc >>= 1;
        if (bs)
        {
            crc ^= 0xA001;
        }
    }
    return crc;
}

void T_form_packet_to_send(unsigned char number, unsigned char command_byte, unsigned char *packet_send)
{

    size_t crc;
    char outstring[10];

    packet_send[0] = 0x01;
    packet_send[1] = number;
    packet_send[2] = 0x02;
    packet_send[3] = command_byte;

    packet_send[4] = 0x17;

    crc = T_CRC16_Compute_with_one_number(number, command_byte);
    sprintf(outstring, "%04lX", crc);

    packet_send[5] = outstring[2];
    packet_send[6] = outstring[3];
    packet_send[7] = outstring[0];
    packet_send[8] = outstring[1];

    packet_send[9] = 0x03;
}

unsigned char T_check_received_packet(unsigned char *packet_received, size_t length)
{

    size_t crc;
    char outstring[10];

    if (length < 10)
        return 0;
    if (length > 32767)
        return 0;

    if (packet_received[0] != 0x01)
        return 0;
    if (packet_received[2] != 0x02)
        return 0;

    if (packet_received[length - 1] != 0x03)
        return 0;
    if (packet_received[length - 6] != 0x17)
        return 0;

    crc = T_CRC16_Compute_with_number(packet_received[1], packet_received + 3, length - 9);

    sprintf(outstring, "%04lX", crc);

    if (packet_received[length - 5] != outstring[2])
        return 0;
    if (packet_received[length - 4] != outstring[3])
        return 0;
    if (packet_received[length - 3] != outstring[0])
        return 0;
    if (packet_received[length - 2] != outstring[1])
        return 0;

    return packet_received[1];
}

unsigned short T_convert_hex_ascii_coded_to_byte(unsigned char *code, unsigned char *res, int length)
{
    int new_len = length / 2;
    unsigned char *buf = code, *ch = res;

    while (new_len--)
    {
        *ch = 0;

        if ((*buf > 47) && (*buf < 58))
            (*ch) += (*buf - 48) * 16;
        else if ((*buf > 64) && (*buf < 71))
            (*ch) += (*buf - 55) * 16;
        else
            return 0;

        buf++;

        if ((*buf > 47) && (*buf < 58))
            (*ch) += *buf - 48;
        else if ((*buf > 64) && (*buf < 71))
            (*ch) += *buf - 55;
        else
            return 0;

        buf++;
        ch++;
    }

    return length / 2;
}

int T_receive_packet_from_port(MY_COM_PORT *mcport, unsigned char command_byte)
{

    unsigned char number_out;
    int stop_byte_found = 0, read_bytes, retnum;

    if (!mcport->if_open_port)
        return -98;

    //form the data packet to communicate with the Cimel Model T control box
    T_form_packet_to_send(mcport->message_number, command_byte, mcport->packet_send);
    mcport->buf = mcport->packet_received;

    //send packet to the com port

    write(mcport->fd, mcport->packet_send, 10);

    //wait for the proper response from the control box

    while (!stop_byte_found)
    {
        retnum = reading_single_port_with_timeout(mcport);

        if (retnum == -3)
            return -99; // error on line. Port closed by reading_single_port_with_timeout

        if (retnum == -2)
            return -1;

        if (retnum == 1)
        {
            if (mcport->buf[-1] == 0x03)
                stop_byte_found = 1;

            mcport->length_ret = mcport->buf - mcport->packet_received;

            if (!stop_byte_found && (mcport->length_ret > 3000))
                return -97;
        }
    }

    //check data bytes consistency
    number_out = T_check_received_packet(mcport->packet_received, mcport->length_ret);

    if (!number_out)
        return 0;

    if (number_out != mcport->message_number)
        return -2;

    mcport->message_number++;
    if (mcport->message_number == 0x80)
        mcport->message_number = 0x21;

    if (command_byte + 0x80 != mcport->packet_received[3])
        return -3;

    //convert the data bytes
    mcport->length_data = T_convert_hex_ascii_coded_to_byte(mcport->packet_received + 4,
                                                            mcport->payload_received, mcport->length_ret - 10);

    if (!mcport->length_data)
        return -4;

    return (mcport->length_data);
}

int T_receive_cimel_time_from_port(MY_COM_PORT *mcport)
{
    struct tm mtim;

    mcport->cimel_time = 0;

    if (T_receive_packet_from_port(mcport, 'A') != 6)
        return 0;

    if ((mcport->payload_received[0] + 2000 <= 2017 || mcport->payload_received[0] + 2000 >= 2050) &&
            (mcport->payload_received[1] < 1 || mcport->payload_received[1] > 12) ||
        (mcport->payload_received[2] < 1 || mcport->payload_received[2] > 31) ||
        (mcport->payload_received[3] < 0 || mcport->payload_received[3] > 23) ||
        (mcport->payload_received[4] < 0 || mcport->payload_received[4] > 59) ||
        (mcport->payload_received[5] < 0 || mcport->payload_received[5] > 59))
        return 0;

    mtim.tm_year = mcport->payload_received[0] + 100;
    mtim.tm_mon = mcport->payload_received[1] - 1;
    mtim.tm_mday = mcport->payload_received[2];
    mtim.tm_hour = mcport->payload_received[3];
    mtim.tm_min = mcport->payload_received[4];
    mtim.tm_sec = mcport->payload_received[5];

    mcport->cimel_time = timegm(&mtim);

    if (!mcport->cimel_time)
        return 0;

    return 1;
}

time_t T_convert_bytes_to_time_t(unsigned char *buf)
{

    struct tm mtim;
    mtim.tm_year = buf[3] / 4 + 100;
    mtim.tm_mon = buf[3] % 4 * 4 + buf[2] / 64 - 1;
    mtim.tm_mday = buf[2] % 64 / 2;
    mtim.tm_hour = buf[2] % 2 * 16 + buf[1] / 16;
    mtim.tm_min = buf[1] % 16 * 4 + buf[0] / 64;
    mtim.tm_sec = buf[0] % 64;

    return timegm(&mtim);
}

unsigned char T_retrieve_new_record(unsigned char *buf, size_t record_size, RECORD_BUFFER *ptr)
{

    size_t i;

    if (buf[0] > 0x7F)
        return 0;

    ptr->record_size = buf[1] + (buf[2] % 64) * 256;

    if (ptr->record_size != record_size)
        return 0;
    if (buf[1] != buf[record_size - 2])
        return 0;
    if (buf[2] != buf[record_size - 1])
        return 0;

    if (buf[record_size - 3] != 0xFE)
        return 0;

    ptr->idbyte = buf[0] % 0x80;

    //   printf ("idbyte = %d\n", ptr->idbyte);

    ptr->record_time = T_convert_bytes_to_time_t(buf + 3);

    ptr->buffer = (unsigned char *)malloc(record_size);

    for (i = 0; i < record_size; i++)
        ptr->buffer[i] = buf[i];

    return 1;
}

int T_receive_header_from_port(MY_COM_PORT *mcport, CIMEL_BUFFER *ptr)
{
    int status;
    short *nums;

    status = T_receive_packet_from_port(mcport, 'G');

    /*
       Here all checks for the size of Header is removed (except it cannot be less than 56 - the smallest ever header of K8)
       We assume that if packet goes thru consistency check, shows id byte as 0x7C and is at least 56 bytes long, than it
        is a valid header. More checks will be done on "aeronet".
       For this program we only need cimel number (bytes 15, 16) and software version (bytes 9,10). They are on the same places in 
       all headers so far (as of this writing, August 5, 2020)

       Now if Cimel modify the size of header again, the current program will continue operating without recompile.

    */

    printf("Header packet Status = %d\n", status);
    if (status < 56)
        return 0;

    if (!T_retrieve_new_record(mcport->payload_received, status, &ptr->header))
        return 0;

    if (ptr->header.idbyte != 0x7C)
    {
        free(ptr->header.buffer);
        return 0;
    }

    nums = (short *)(ptr->header.buffer + 15);
    ptr->cimel_number = nums[0];

    // form "EPROM" from soft_ver_major (ptr->header.buffer[9]) and soft_ver_manor(ptr->header.buffer[10])
    sprintf(ptr->eprom, "SP810%02X%02X", ptr->header.buffer[9], ptr->header.buffer[10]);

    printf("Received T header with Cimel number %d EPROM %s\n", ptr->cimel_number, ptr->eprom);

    ptr->if_header = 1;

    return 1;
}

int T_retrieve_k8_buffer(MY_COM_PORT *mcport, CIMEL_BUFFER *ptr, int max_num)
{
    unsigned char command_byte;
    int continue_retrieval = 1;

    if (!ptr->if_header)
    {
        if (!T_receive_header_from_port(mcport, ptr))
            return 0;
    }

    if (!ptr->allocated_records)
    {
        ptr->allocated_records = 20;

        ptr->records = (RECORD_BUFFER *)malloc(sizeof(RECORD_BUFFER) * ptr->allocated_records);
    }
    command_byte = 'C';

    while (continue_retrieval)
    {

        if (T_receive_packet_from_port(mcport, command_byte) < 1)
            continue_retrieval = 0;
        else
        {

            if (!T_retrieve_new_record(mcport->payload_received, mcport->length_data, ptr->records + ptr->num_records))
                continue_retrieval = 0;
            else
            {
                if (command_byte == 'C')
                    command_byte = 'D';

                if (ptr->records[ptr->num_records].record_time <= mcport->last_time_T)
                    continue_retrieval = 0;

                if (continue_retrieval)
                {
                    printf("num = %d  idbyte = %d  time = %s", ptr->num_records,
                           ptr->records[ptr->num_records].idbyte, ctime(&ptr->records[ptr->num_records].record_time));

                    ptr->num_records++;
                    if (max_num && (ptr->num_records == max_num))
                        continue_retrieval = 0;
                    else if (ptr->num_records == ptr->allocated_records)
                    {
                        ptr->allocated_records += 20;
                        ptr->records = (RECORD_BUFFER *)realloc(ptr->records, sizeof(RECORD_BUFFER) * ptr->allocated_records);
                    }
                }
            }
        }
    }

    if (!ptr->num_records)
    {
        free_cimel_buffer(ptr);
        return 0;
    }

    mcport->last_time_T = ptr->records->record_time;

    return 1;
}

int T_retrieve_k8_buffer_data_only(MY_COM_PORT *mcport, CIMEL_BUFFER *ptr, int max_num)
{
    unsigned char command_byte;
    int continue_retrieval = 1, the_packet, the_record;
    time_t pc_time;

    command_byte = 'C';

    while (continue_retrieval)
    {
        the_packet = T_receive_packet_from_port(mcport, command_byte);
        //printf ("Packet code = %d\n", the_packet);

        if (the_packet < 1)
        {

            continue_retrieval = 0;
        }
        else
        {

            ptr->allocated_records++;
            ptr->records = (RECORD_BUFFER *)realloc(ptr->records, sizeof(RECORD_BUFFER) * ptr->allocated_records);

            the_record = T_retrieve_new_record(mcport->payload_received, mcport->length_data, ptr->records + ptr->num_records);

            //printf ("Record code = %d\n", the_record);
            if (!the_record)
                continue_retrieval = 0;
            else
            {

                if (command_byte == 'C')
                    command_byte = 'D';

                if (ptr->records[ptr->num_records].record_time <= mcport->last_time_T)
                    continue_retrieval = 0;

                if (continue_retrieval)
                {
                    printf("num = %d  idbyte = %d  time = %s", ptr->num_records,
                           ptr->records[ptr->num_records].idbyte, ctime(&ptr->records[ptr->num_records].record_time));

                    ptr->num_records++;
                    if (max_num && (ptr->num_records == max_num))
                        continue_retrieval = 0;
                }
            }
        }
    }

    if (!ptr->num_records)
    {
        printf ("Found no new records\n");
        free_cimel_buffer(ptr);
        return 0;
    }

    mcport->last_time_T = ptr->records->record_time;
    pc_time = time(NULL);
    printf("Retrieved K8 buffer with %d records  %s", ptr->num_records, ctime(&pc_time));

    return 1;
}

int T_save_k8_buffer_on_disk(char *dir_name, CIMEL_BUFFER *ptr)
{
    int i;
    FILE *out;

    if (!ptr->if_header)
        return 0;

    sprintf(ptr->real_file_name, "%s/%s", dir_name, ptr->file_name);

    out = fopen(ptr->real_file_name, "w");

    if (out == NULL)
        return 0;

    fwrite(ptr->header.buffer, 1, ptr->header.record_size, out);
    for (i = 0; i < ptr->num_records; i++)
        fwrite(ptr->records[i].buffer, 1, ptr->records[i].record_size, out);

    fclose(out);
    return 1;
}

time_t T_read_k8_buffer_from_disk(char *dir_name, CIMEL_BUFFER *ptr)
{
    FILE *in;
    unsigned char *buffer, *buf, *bufend;
    struct stat bufff;

    size_t file_read, rec_size;

    short *nums;

    int num_recs = 0, ii, head_size, end_read;

    init_cimel_buffer(ptr);

    sprintf(ptr->real_file_name, "%s/%s", dir_name, ptr->file_name);

    if (stat(ptr->real_file_name, &bufff))
        return 0;

    if (bufff.st_size < 56)
        return 0;

    in = fopen(ptr->real_file_name, "r");

    if (in == NULL)
        return 0;

    buffer = (unsigned char *)malloc(bufff.st_size);
    file_read = fread(buffer, 1, bufff.st_size, in);
    fclose(in);

    //   printf("file_read = %ld  file_size = %ld\n", file_read, file_size);

    if (file_read != bufff.st_size)
    {
        free(buffer);
        return 0;
    }

    buf = buffer;
    end_read = 0;
    bufend = buffer + bufff.st_size;

    while (!end_read && (buf < bufend))
    {
        rec_size = buf[1] + (buf[2] % 64) * 256;

        if ((buf - buffer) + rec_size - 1 >= bufff.st_size)
            end_read = 1;
        else if ((buf[1] != buf[rec_size - 2]) || (buf[2] != buf[rec_size - 1]))
            end_read = 1;
        else
        {
            if (!ptr->if_header)
            {
                if (!T_retrieve_new_record(buf, rec_size, &ptr->header))
                {
                    free(buffer);
                    return 0;
                }

                if (ptr->header.idbyte != 0x7C)
                {
                    free(ptr->header.buffer);
                    free(buffer);
                    return 0;
                }

                nums = (short *)(ptr->header.buffer + 15);
                ptr->cimel_number = nums[0];

                sprintf(ptr->eprom, "SP810%02X%02X", ptr->header.buffer[9], ptr->header.buffer[10]);
                ptr->if_header = 1;
                buf += rec_size;
            }
            else
            {
                ptr->allocated_records++;
                ptr->records = (RECORD_BUFFER *)realloc(ptr->records, sizeof(RECORD_BUFFER) * ptr->allocated_records);
                if (!T_retrieve_new_record(buf, rec_size, ptr->records + ptr->num_records))
                    end_read = 1;
                else
                {
                    ptr->num_records++;
                    buf += rec_size;
                }
            }
        }
    }

    free(buffer);

    return ptr->records->record_time;
}

// Specific V4 and V5 functions

void V5_put_sys_time_to_buffer(time_t *sys_time, unsigned char *buffer)
{
    struct tm mtim;
    gmtime_r(sys_time, &mtim);
    buffer[0] = mtim.tm_year;
    buffer[1] = mtim.tm_mon + 1;
    buffer[2] = mtim.tm_mday;
    buffer[3] = mtim.tm_hour;
    buffer[4] = mtim.tm_min;
    buffer[5] = mtim.tm_sec;
}

time_t V5_get_sys_time_from_buffer(unsigned char *buffer)
{
    struct tm mtim;

    mtim.tm_year = buffer[0];
    mtim.tm_mon = buffer[1] - 1;
    mtim.tm_mday = buffer[2];
    mtim.tm_hour = buffer[3];
    mtim.tm_min = buffer[4];
    mtim.tm_sec = buffer[5];

    return timegm(&mtim);
}

unsigned char V5_convert_char_to_BYTE(unsigned char ch)
{
    if (ch < 48)
        return 0;
    if (ch < 58)
        return ch - 48;
    if (ch < 65)
        return 0;
    if (ch < 71)
        return ch - 55;
    return 0;
}

unsigned char V5_get_decimall(unsigned char ch)
{

    return (ch / 16) * 10 + ch % 16;
}

unsigned char V5_convert_block(unsigned char *buffer, unsigned char *result, int num)
{
    unsigned char *buf = buffer, *res = result, *stop_byte = buffer + num;

    while (buf != stop_byte)
    {
        *res = 16 * V5_convert_char_to_BYTE(buf[0]) + V5_convert_char_to_BYTE(buf[1]);

        res++;
        buf += 2;
    }
    return result[0];
}

time_t V5_get_cimel_time(unsigned char *buffer, int length)
{
    struct tm mtim;

    if (length != 12)
        return 0;

    mtim.tm_hour = (buffer[0] - 48) * 10 + buffer[1] - 48;
    mtim.tm_min = (buffer[2] - 48) * 10 + buffer[3] - 48;
    mtim.tm_sec = (buffer[4] - 48) * 10 + buffer[5] - 48;

    mtim.tm_mday = (buffer[6] - 48) * 10 + buffer[7] - 48;
    mtim.tm_mon = (buffer[8] - 48) * 10 + buffer[9] - 48 - 1;
    mtim.tm_year = (buffer[10] - 48) * 10 + buffer[11] - 48 + 100;
    return timegm(&mtim);
}

int V5_check_buffer_for_checksum(MY_COM_PORT *mcport)
{

    unsigned char *buf, checksum, checks;

    //rintf ("\nChecksum shows :  %c%c\n", mcport->end[1], mcport->end[2]);

    V5_convert_block(mcport->end + 1, &checksum, 2);

    buf = mcport->begin;
    checks = 0;

    while (buf != mcport->end + 1)
        checks ^= *buf++;

    return (checks == checksum);
}

void V5_init_port_receiption(MY_COM_PORT *mcport)
{
    mcport->header_flag = mcport->time_header_flag =
        mcport->empty_event_count = mcport->time_correction_flag =
            mcport->time_count = 0;

    mcport->previous_time = 0;
}

void V5_wait_for_new_packet(MY_COM_PORT *mcport)
{
    mcport->begin = mcport->end = NULL;
    mcport->buf = mcport->packet_received;
}

void V5_send_request_to_cimel(MY_COM_PORT *mcport, char *request, int num_bytes)
{
    write(mcport->fd, request, num_bytes);
    V5_wait_for_new_packet(mcport);
}

/*
return : 
-99 : packet received, sequence not right : abort K7 waiting (sends "ZTZ")
0 - packet not completed
-1 - packet completed, checksum is wrong,  sends "uT" to repeat last packet

1 - received header, sends "HT"  sets header_flag to 1 decide on time_correction_flag
2 - received header time (1st), sets time_header_flag to 1, sends jT - start collecting data

3 - received cimel times (3 times) 

4 - received cimel time (3rd time) or no correction and k7 completed

5 - empty event increment sends jT - continue collecting data. 

6 - received data record
-2 - recived data records with errors,  sends jT - continue collecting data. 

*/

int V5_check_if_packet_completed(MY_COM_PORT *mcport, AERO_EXCHANGE *aerex)
{

    int header_size, i;
    time_t correct_time;
    struct tm mtim;
    char time_correction_string[20];

    if ((mcport->end == NULL) || (mcport->begin == NULL) || (mcport->buf - mcport->end < 3))
        return 0;

    if (!V5_check_buffer_for_checksum(mcport))
    {
        V5_send_request_to_cimel(mcport, "uT", 2);
        return -1;
    }

    mcport->length_ret = mcport->end - mcport->begin;

    if (mcport->begin[1] == 'S')
    {
        if (mcport->header_flag || (mcport->length_ret % 2))
        {
            V5_send_request_to_cimel(mcport, "ZTZ", 3);
            V5_init_port_receiption(mcport);
            return -99;
        }
        mcport->length_data = (mcport->length_ret - 12) / 2;
        V5_convert_block(mcport->begin + 12, mcport->payload_received, mcport->length_ret - 12);

        mcport->header_flag = 1;

        V5_send_request_to_cimel(mcport, "HT", 2);

        return 1;
    }

    if ((mcport->begin[1] == 'h') || (mcport->begin[1] == 'H') || (mcport->begin[1] == 'R'))
    {
        if (!mcport->header_flag)
        {
            V5_send_request_to_cimel(mcport, "ZTZ", 3);
            V5_init_port_receiption(mcport);
            return -98;
        }

        if (!mcport->time_header_flag)
        {
            mcport->cimel_time = V5_get_cimel_time(mcport->begin + 2, mcport->length_ret - 2);
            mcport->time_header_flag = 1;
            V5_send_request_to_cimel(mcport, "jT", 2);
            return 2;
        }

        if (!mcport->time_correction_flag)
        {
            V5_send_request_to_cimel(mcport, "ZTZ", 3);
            V5_init_port_receiption(mcport);
            return 4;
        }
        if (mcport->time_count == 1)
        {
            mcport->cimel_time = V5_get_cimel_time(mcport->begin + 2, mcport->length_ret - 2);

            if (aerex->status && aerex->good_clock)

            {
                correct_time = time(NULL);

                gmtime_r(&correct_time, &mtim);

                sprintf(time_correction_string, "R1234%02d%02d%02d%02d%02d%02dT",
                        mtim.tm_hour, mtim.tm_min,
                        mtim.tm_sec, mtim.tm_mday,
                        mtim.tm_mon + 1, mtim.tm_year % 100);
                mcport->begin = mcport->end = NULL;
                mcport->buf = mcport->packet_received;

                V5_send_request_to_cimel(mcport, time_correction_string, 18);
                mcport->time_count++;
                return 3;
            }
        }

        if (mcport->time_count == 3)
        {
            V5_send_request_to_cimel(mcport, "ZTZ", 3);
            V5_init_port_receiption(mcport);
            return 4;
        }

        mcport->time_count++;
        V5_send_request_to_cimel(mcport, "HT", 2);

        return 3;
    }

    if (mcport->begin[1] != '1')
    {
        V5_send_request_to_cimel(mcport, "ZTZ", 3);
        V5_init_port_receiption(mcport);
        return -101 - mcport->begin[1];
    }

    if (!mcport->header_flag || (!mcport->time_header_flag))
    {
        V5_send_request_to_cimel(mcport, "ZTZ", 3);
        V5_init_port_receiption(mcport);
        return -96;
    }

    mcport->length_data = (mcport->length_ret - 2) / 2;
    V5_convert_block(mcport->begin + 2, mcport->payload_received, mcport->length_ret - 2);

    if ((mcport->payload_received[1] != mcport->length_data) || (mcport->payload_received[mcport->length_data - 1] != mcport->length_data) || (mcport->payload_received[0] == 0) || (mcport->payload_received[0] == 255))
    {
        mcport->empty_event_count++;
        if (mcport->empty_event_count == 2)
        {
            if (mcport->time_correction_flag)
            {
                V5_send_request_to_cimel(mcport, "HT", 2);
                return 3;
            }
            V5_send_request_to_cimel(mcport, "ZTZ", 3);
            V5_init_port_receiption(mcport);
            return 4;
        }
        V5_send_request_to_cimel(mcport, "jT", 2);
        return 5;
    }
    mcport->empty_event_count = 0;

    mtim.tm_year = V5_get_decimall(mcport->payload_received[4]) + 100;
    mtim.tm_mon = V5_get_decimall(mcport->payload_received[5]) - 1;
    mtim.tm_mday = V5_get_decimall(mcport->payload_received[6]);
    mtim.tm_hour = V5_get_decimall(mcport->payload_received[7]);
    mtim.tm_min = V5_get_decimall(mcport->payload_received[8]);
    mtim.tm_sec = V5_get_decimall(mcport->payload_received[9]);

    mcport->cimel_time = timegm(&mtim);

    if ((mcport->cimel_time <= mcport->last_time_5) || ((mcport->previous_time != 0) && (mcport->cimel_time > mcport->previous_time)))
    {
        if (mcport->time_correction_flag)
        {
            V5_send_request_to_cimel(mcport, "HT", 2);
            return 3;
        }
        V5_send_request_to_cimel(mcport, "ZTZ", 3);
        V5_init_port_receiption(mcport);
        return 4;
    }

    mcport->previous_time = mcport->cimel_time;
    V5_send_request_to_cimel(mcport, "jT", 2);
    return 6;
}

/*

returns:
0 - no events or normal flow of data download, or aborted K7 file so continue
1 - reached time interval upon the above condition

2 - completed K7 buffer
3 - reached time interval upon the above condition

8 - received proper header , will continue
 

*/

int V5_main_loop_cycle(MY_COM_PORT *mcport, AERO_EXCHANGE *aerex, CIMEL_BUFFER *k7_buffer)
{

    int retval, i, status;
    time_t new_time, correct_time;
    RECORD_BUFFER *ptr;

    retval = reading_single_port_with_timeout(mcport);

    switch (retval)
    {
    case 0:
    case -2:

        /*
perform timeout. if time_interval reached, then action - upload hourle and/or daily file 

*/

        new_time = time(NULL);
        if (new_time > mcport->check_time + mcport->time_interval)
        {
            mcport->check_time = new_time;
            return 1;
        }

        return 0;

        break;

    case -3:
        //printf("Exit\n");
        exit(0);

    case 1:
        status = V5_check_if_packet_completed(mcport, aerex);

        if (status)
        {
            new_time = time(NULL);

            if (status == 1)
            {

                k7_buffer->header.buffer = (unsigned char *)malloc(256);
                k7_buffer->header.record_size = 256;
                k7_buffer->if_header = 1;

                memcpy(k7_buffer->header.buffer, mcport->payload_received, mcport->length_data);
                for (i = mcport->length_data; i < 256; i++)
                    k7_buffer->header.buffer[i] = 0;

                return 0;
            }
            else if (status == 2)
            {
                k7_buffer->header.record_time = mcport->cimel_time;

                V5_put_sys_time_to_buffer(&mcport->cimel_time, k7_buffer->header.buffer + 144);
                V5_put_sys_time_to_buffer(&new_time, k7_buffer->header.buffer + 150);
                sprintf(k7_buffer->header.buffer + 162, "%s", mcport->hostname);
                sprintf(k7_buffer->header.buffer + 203, "%s", mcport->program_version);

                if (aerex->status && aerex->good_clock)

                {

                    correct_time = time(NULL);

                    V5_put_sys_time_to_buffer(&correct_time, k7_buffer->header.buffer + 156);
                    if ((correct_time > mcport->cimel_time + 10) || (correct_time < mcport->cimel_time - 10))
                    {
                        mcport->time_correction_flag = 1;
                    }

                    printf("Time difference = %ld", correct_time - mcport->cimel_time);
                    if (mcport->time_correction_flag)
                        printf(" Time correction  suggested");
                    printf("\n");
                }

                else
                {
                    for (i = 156; i < 162; i++)
                        k7_buffer->header.buffer[156] = 255;
                }

                k7_buffer->cimel_number = k7_buffer->header.buffer[3] * 256 + k7_buffer->header.buffer[4];
                for (i = 0; i < 8; i++)
                    k7_buffer->eprom[i] = k7_buffer->header.buffer[i + 128];
                k7_buffer->eprom[8] = '\0';

                printf("Port %s - header %s  %d\n", mcport->port_name, k7_buffer->eprom, k7_buffer->cimel_number);
                return 8;
            }
            else if (status == 6)
            {
                k7_buffer->allocated_records++;
                k7_buffer->records = (RECORD_BUFFER *)realloc(k7_buffer->records, sizeof(RECORD_BUFFER) * k7_buffer->allocated_records);

                ptr = k7_buffer->records + k7_buffer->num_records;

                ptr->buffer = (unsigned char *)malloc(mcport->length_data);
                memcpy(ptr->buffer, mcport->payload_received, mcport->length_data);
                ptr->idbyte = ptr->buffer[0];
                ptr->record_size = mcport->length_data;
                ptr->record_time = mcport->cimel_time;

                k7_buffer->num_records++;

                printf("num = %d  idbyte = %d  time = %s", k7_buffer->num_records, ptr->idbyte, ctime(&ptr->record_time));

                return 0;
            }
            else if ((status == -1) || (status == -2) || (status == 3) || (status == 5))
                return 0;
            else if (status < -80)
            {
                free_cimel_buffer(k7_buffer);

                if (new_time > mcport->check_time + mcport->time_interval)
                {
                    mcport->check_time = new_time;
                    return 1;
                }

                return 0;
            }
            else if (status == 4)
            {

                if (k7_buffer->num_records)
                    mcport->last_time_5 = k7_buffer->records->record_time;

                if (new_time > mcport->check_time + mcport->time_interval)
                {
                    mcport->check_time = new_time;
                    return 3;
                }

                return 2;
            }
        }
        return 0;
        break;
    }
    return 0;
}

int V5_save_k7_buffer_on_disk(char *dir_name, CIMEL_BUFFER *ptr)
{
    int i;
    FILE *out;

    if (!ptr->if_header)
        return 0;

    sprintf(ptr->real_file_name, "%s/%s", dir_name, ptr->file_name);

    out = fopen(ptr->real_file_name, "w");

    if (out == NULL)
        return 0;

    fwrite(ptr->header.buffer, 1, ptr->header.record_size, out);
    for (i = 0; i < ptr->num_records; i++)
        fwrite(ptr->records[i].buffer, 1, ptr->records[i].record_size, out);

    fclose(out);
    return 1;
}

time_t V5_read_k7_buffer_from_disk(char *dir_name, CIMEL_BUFFER *ptr)
{
    FILE *in;
    unsigned char *buffer, *buf, *bufend;

    struct stat bufff;
    RECORD_BUFFER *record;
    size_t file_read, rec_size;

    struct tm mtim;

    int end_read, i;

    init_cimel_buffer(ptr);

    sprintf(ptr->real_file_name, "%s/%s", dir_name, ptr->file_name);

    if (stat(ptr->real_file_name, &bufff))
        return 0;

    if (bufff.st_size <= 256)
        return 0;

    in = fopen(ptr->real_file_name, "r");

    if (in == NULL)
        return 0;
    buffer = (unsigned char *)malloc(bufff.st_size);
    file_read = fread(buffer, 1, bufff.st_size, in);
    fclose(in);

    //printf("file_read = %ld  file_size = %ld\n", file_read, bufff.st_size);

    if (file_read != bufff.st_size)
    {
        free(buffer);
        return 0;
    }

    ptr->header.buffer = (unsigned char *)malloc(256);
    ptr->header.record_size = 256;
    memcpy(ptr->header.buffer, buffer, 256);
    ptr->header.record_time = V5_get_sys_time_from_buffer(ptr->header.buffer + 144);
    ptr->cimel_number = ptr->header.buffer[3] * 256 + ptr->header.buffer[4];
    for (i = 0; i < 8; i++)
        ptr->eprom[i] = ptr->header.buffer[i + 128];
    ptr->eprom[8] = '\0';

    ptr->if_header = 1;

    buf = buffer + 256;
    end_read = 0;

    bufend = buffer + bufff.st_size;

    while (!end_read && (buf < bufend))
    {

        rec_size = buf[1];
        if (buf + rec_size >= bufend)
            end_read = 1;
        else if (buf[rec_size - 1] != rec_size)
            end_read = 1;
        else
        {
            /* code */
            ptr->allocated_records++;
            ptr->records = (RECORD_BUFFER *)realloc(ptr->records, sizeof(RECORD_BUFFER) * ptr->allocated_records);
            record = ptr->records + ptr->num_records;
            record->buffer = (unsigned char *)malloc(rec_size);
            memcpy(record->buffer, buf, rec_size);

            mtim.tm_year = V5_get_decimall(buf[4]) + 100;
            mtim.tm_mon = V5_get_decimall(buf[5]) - 1;
            mtim.tm_mday = V5_get_decimall(buf[6]);
            mtim.tm_hour = V5_get_decimall(buf[7]);
            mtim.tm_min = V5_get_decimall(buf[8]);
            mtim.tm_sec = V5_get_decimall(buf[9]);
            record->record_time = timegm(&mtim);

            record->idbyte = record->buffer[0];
            record->record_size = rec_size;

            ptr->num_records++;

            buf += rec_size;
        }
    }

    free(buffer);

    return ptr->records->record_time;
}
