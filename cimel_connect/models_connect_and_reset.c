#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <termios.h>

#include <sys/ioctl.h>

#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <curl/curl.h>
#include <fcntl.h>
#include <errno.h>

#include "models_port.h"

int main(int argc, char **argv)
{

    /*
Common variables
*/
    MY_COM_PORT mcport;
    char usb_port[100], usb_reset_command[300];
    int cimel_model, retval, dev_init_5, dev_init_T, read_port, if_local = 1, iarg, reset_counter = 0; // if_local, 1 : do not operate hologram modem, 0 - operate it.
    char backup_dir[500], *homedir = getenv("HOME"), log_dir[200], log_file[200], message_text[1000],
                          *username = getenv("USER");
    char file_nameh[400], file_named[400], file_namem[400];

    int upload_switch, upload_switch_m, upload_switch_d, upload_switch_h, init_upload;
    struct stat bufff;

    FILE *out_log;

    /*
V5 variables
*/
    CIMEL_BUFFER k7b, k7bm, k7bh, k7bd;
    struct tm mtim;
    time_t pc_time, new_time, stop_time, log_day, log_day1, pc_time1;
    time_t last_time_5;
    AERO_EXCHANGE aerex;
    /*
T variables
*/
    CIMEL_BUFFER k8b, k8bh, k8bd;
    time_t last_time_T;

    /*
set up log directory, log day and log file

*/

    sprintf(log_dir, "%s/cimel_logs", homedir);

    pc_time = time(NULL);
    log_day = pc_time / 86400;

    gmtime_r(&pc_time, &mtim);

    sprintf(log_file, "%s/connection_log_%d_%02d_%02d.txt",
            log_dir, mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);

    sprintf(message_text, "\n-------\n%s started at %s\n", argv[0], asctime(gmtime(&pc_time)));
    output_message_to_log(log_file, message_text);

    /*
Step 0.  decide if local
*/
    // form usb reset command
    if (!strcmp(argv[1],"local")) 
    {

        output_message_to_log(log_file, "Use local mode\n");

        if_local = 1;
}
else
{
     if_local = 0;

sprintf (usb_reset_command,"sh /home/%s/AeroLinux/controls/USB_handler.sh", username); 
    
        sprintf(message_text, "Use hologram mode, usb reset command = [%s]\n", usb_reset_command);
        output_message_to_log(log_file, message_text);
}

    // Checking for the previous 10 days log files

    init_upload = 1;

    for (log_day1 = log_day - 1; log_day1 > log_day - 10; log_day1--)
    {
        pc_time1 = log_day1 * 86400;
        gmtime_r(&pc_time1, &mtim);
        sprintf(log_file, "%s/connection_log_%d_%02d_%02d.txt",
                log_dir, mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
        if (!stat(log_file, &bufff))
        {
            if (!if_local && init_upload)
            {

                init_upload = 0;
                if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                    exit(0);
            }

            upload_daily_connection_log_to_ftp(log_file, username);
        }
    }

    if (!if_local && (!init_upload))
    {
        system("sudo hologram network disconnect");
    }

    /*
Step 1.  Form the com_port

*/

    gethostname(mcport.hostname, 39);
    mcport.hostname[39] = '\0';
    strcpy(mcport.program_version, PROG_VERSION);
    mcport.packet_timeout = 15;

    mcport.if_open_port = 0;
    mcport.if_T_port_already_open = 0;
    strcpy(mcport.port_name, usb_port);

    /*
Step 2.
if argvs exist, redefine backup_dir and time_interval

*/

    sprintf(backup_dir, "%s/backup", homedir);
    mcport.time_interval = 900; // default : 15 minutes

    // printf("argc = %d\n", argc);

    if (argc > 2)
        for (iarg = 2; iarg < argc; iarg++)
        {
            if (!strncmp(argv[iarg], "dir=", 4))
                strcpy(backup_dir, argv[iarg] + 4);
            else if (!strncmp(argv[iarg], "int=", 4))
                mcport.time_interval = atoi(argv[iarg] + 4);
        }

    /*
Step 3.

define last times and cimel numbers previously connected models T and V5

*/

    strcpy(k7b.file_name, "last_time.k7");
    printf("Now will read last_time.k7 file\n");
    last_time_5 = V5_read_k7_buffer_from_disk(homedir, &k7b);

    if (last_time_5)
    {
        sprintf(message_text, "Found last saved time of V5 : %sPreviously used V5 Cimel number = %d, EPROM = %s\n",
                asctime(gmtime(&last_time_5)), k7b.cimel_number, k7b.eprom);
        output_message_to_log(log_file, message_text);
        mcport.cimel_number_5 = k7b.cimel_number;
        strcpy(mcport.eprom_5, k7b.eprom);
        mcport.last_time_5 = last_time_5;

        pc_time = time(NULL);
        gmtime_r(&pc_time, &mtim);

        sprintf(k7bm.file_name, "%s_%04d_%d%02d%02d_%02d%02d.K7", k7b.eprom, k7b.cimel_number,
                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour, mtim.tm_min);
        init_cimel_buffer(&k7bm);
        combine_cimel_buffers(&k7bm, &k7b);

        sprintf(k7bh.file_name, "%s_%04d_%d%02d%02d_%02d.K7", k7b.eprom, k7b.cimel_number,
                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour);
        init_cimel_buffer(&k7bh);

        sprintf(k7bd.file_name, "%s_%04d_%d%02d%02d.K7", k7b.eprom, k7b.cimel_number,
                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
        V5_read_k7_buffer_from_disk(backup_dir, &k7bd);

        free_cimel_buffer(&k7b);
    }
    else
    {
        output_message_to_log(log_file, "last_time.k7 not found. No previously used V5 instrument detected\n");
        mcport.cimel_number_5 = -1;
        mcport.last_time_5 = 0;
        init_cimel_buffer(&k7bm);
        init_cimel_buffer(&k7bh);
        init_cimel_buffer(&k7bd);
        init_cimel_buffer(&k7b);
    }

    strcpy(k8b.file_name, "last_time.k8");
    output_message_to_log(log_file, "Now will read last_time.k8 file\n");
    last_time_T = T_read_k8_buffer_from_disk(homedir, &k8b);

    if (last_time_T)
    {
        sprintf(message_text, "Found last saved time of T : %sPreviously used T Cimel number = %d EPROM = %s\n",
                asctime(gmtime(&last_time_T)), k8b.cimel_number, k8b.eprom);
        output_message_to_log(log_file, message_text);
        mcport.cimel_number_T = k8b.cimel_number;
        strcpy(mcport.eprom_T, k8b.eprom);
        mcport.last_time_T = last_time_T;

        pc_time = time(NULL);
        gmtime_r(&pc_time, &mtim);

        sprintf(k8bh.file_name, "%s_%04d_%d%02d%02d_%02d.K8", k8b.eprom, k8b.cimel_number,
                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour);
        init_cimel_buffer(&k8bh);

        sprintf(k8bd.file_name, "%s_%04d_%d%02d%02d.K8", k8b.eprom, k8b.cimel_number,
                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
        T_read_k8_buffer_from_disk(NULL, &k8bd);

        free_cimel_buffer(&k8b);
    }
    else
    {
        output_message_to_log(log_file, "last_time.k8 not found. No previously used T instrument detected\n");

        mcport.cimel_number_T = -1;
        mcport.last_time_T = 0;

        init_cimel_buffer(&k8bh);
        init_cimel_buffer(&k8bd);
    }

    /*
Step 4.
start main loop
*/

    output_message_to_log(log_file, "Initially check for T instrument at the port\n");
    cimel_model = _MODEL_T_;

    while (1)
    {

        if (cimel_model == _MODEL_T_)
        {
            pc_time = time(NULL);
            gmtime_r(&pc_time, &mtim);

            log_day1 = pc_time / 86400;
            if (log_day1 != log_day)
            {
                if (!if_local)
                {
                    if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                    {
                        upload_daily_connection_log_to_ftp(log_file, username);
                        exit(0);
                    }
                }
                upload_daily_connection_log_to_ftp(log_file, username);
                if (!if_local)
                    system("sudo hologram network disconnect");
                sprintf(log_file, "%s/connection_log_%d_%02d_%02d.txt",
                        log_dir, mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
                log_day = log_day1;
            }

            if (!mcport.if_T_port_already_open)
            {
                init_cimel_buffer(&k8b);

                if (!define_usb_com_port(usb_port, log_file))
                {
                    sprintf(message_text, "No Serial - USB ports are detected\nProgram %s stop\n", argv[0]);
                    output_message_to_log(log_file, message_text);
                    exit(0);
                }
                strcpy(mcport.port_name, usb_port);

                if (!open_my_com_port(&mcport, _MODEL_T_, log_file))
                {
                    output_message_to_log(log_file, "Port cannot be opened. Stop the program\n");
                    exit(0);
                }

                if (!T_receive_header_from_port(&mcport, &k8b, log_file))
                {
                    if (!if_local)
                    {
                        sprintf(message_text, "%d resets\n", reset_counter);
                        output_message_to_log(log_file, message_text);
                    }
                    pc_time = time(NULL);
                    sprintf(message_text, "T instrument appears disconnected\nWill assume V5 and switch to waiting mode %s", asctime(gmtime(&pc_time)));
                    output_message_to_log(log_file, message_text);
                    close_my_port(&mcport);
                    cimel_model = _MODEL_5_;
                    if (!open_my_com_port(&mcport, _MODEL_5_, log_file))
                    {
                        output_message_to_log(log_file, "Port cannot be opened. Stop the program\n");
                        exit(0);
                    }
                    V5_wait_for_new_packet(&mcport);
                    V5_init_port_receiption(&mcport);
                }
            }

            if (mcport.if_T_port_already_open)
            {

                if (mcport.cimel_number_T == -1)
                    dev_init_T = 1;

                else if ((mcport.cimel_number_T != k8b.cimel_number) ||
                         strcmp(mcport.eprom_T, k8b.eprom))
                {

                    T_save_k8_buffer_on_disk(homedir, &k8bd);

                    if (!if_local)
                    {
                        if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                            exit(0);
                    }

                    libcurl_upload_cimel_buffer_to_https(&k8bd, log_file);

                    if (!if_local)
                    {

                        output_message_to_log(log_file, "Will disconnect modem\n");

                        pc_time = time(NULL);
                        system("sudo hologram network disconnect");
                        stop_time = time(NULL);
                        sprintf(message_text, "Modem disconnected after %d seconds\n", stop_time - pc_time);
                        output_message_to_log(log_file, message_text);
                    }

                    free_cimel_buffer(&k8bh);
                    free_cimel_buffer(&k8bd);
                    dev_init_T = 2;
                }
                else
                    dev_init_T = 0;

                if (dev_init_T)
                {
                    sprintf(message_text, "Redefined T Cimel number = %d  eprom = %s\n", k8b.cimel_number, k8b.eprom);
                    output_message_to_log(log_file, message_text);
                    mcport.cimel_number_T = k8b.cimel_number;
                    strcpy(mcport.eprom_T, k8b.eprom);
                    mcport.last_time_T = 0;

                    sprintf(k8bh.file_name, "%s_%04d_%d%02d%02d_%02d.K8", k8b.eprom, k8b.cimel_number,
                            mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour);
                    sprintf(k8bd.file_name, "%s_%04d_%d%02d%02d.K8", k8b.eprom, k8b.cimel_number,
                            mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
                }
                upload_switch = upload_switch_m = upload_switch_h = upload_switch_d = 0;

                pc_time = time(NULL);
                gmtime_r(&pc_time, &mtim);

                log_day1 = pc_time / 86400;
                if (log_day1 != log_day)
                {
                    if (!if_local)
                    {
                        if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                        {
                            upload_daily_connection_log_to_ftp(log_file, username);
                            exit(0);
                        }
                    }
                    upload_daily_connection_log_to_ftp(log_file, username);
                    if (!if_local)
                        system("sudo hologram network disconnect");
                    sprintf(log_file, "%s/connection_log_%d_%02d_%02d.txt",
                            log_dir, mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
                    log_day = log_day1;
                }

                sprintf(message_text, "Will try to retreive K8 buffer %s", asctime(gmtime(&pc_time)));
                output_message_to_log(log_file, message_text);
                if (T_retrieve_k8_buffer_data_only(&mcport, &k8b, 17000, log_file))
                {

                    T_save_k8_buffer_on_disk(homedir, &k8b);

                    if (k8b.if_header)
                        upload_switch_m = upload_switch = 2;
                    else
                        upload_switch_m = 1;

                    combine_cimel_buffers(&k8bh, &k8b);
                    combine_cimel_buffers(&k8bd, &k8b);

                    T_save_k8_buffer_on_disk(backup_dir, &k8bd);
                }
                close_my_port(&mcport);

                sprintf(file_nameh, "%s_%04d_%d%02d%02d_%02d.K8", mcport.eprom_T, mcport.cimel_number_T,
                        mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour);

                if (strcmp(file_nameh, k8bh.file_name))
                {
                    if (k8bh.if_header)
                        upload_switch_h = upload_switch = 2;
                    else
                        upload_switch_h = 1;

                    strcpy(k8bh.file_name, file_nameh);
                }
                sprintf(file_named, "%s_%04d_%d%02d%02d.K8", mcport.eprom_T, mcport.cimel_number_T,
                        mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
                if (strcmp(file_named, k8bd.file_name))
                {

                    if (k8bd.if_header)
                        upload_switch_d = upload_switch = 2;
                    else
                        upload_switch_d = 1;
                }

                if (upload_switch)
                {

                    if (!if_local)
                    {
                        if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                            exit(0);
                    }

                    if (upload_switch_m == 2)
                    {
                        sprintf(k8b.file_name, "%s_%04d_%d%02d%02d_%02d%02d.K8", k8b.eprom, k8b.cimel_number,
                                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour, mtim.tm_min);

                        sprintf(message_text, "Will upload %s\n", k8b.file_name);
                        output_message_to_log(log_file, message_text);

                        libcurl_upload_cimel_buffer_to_https(&k8b, log_file);

                        strcpy(k8b.file_name, "last_time.k8");
                        free_cimel_buffer(&k8b);
                    }
                    if (upload_switch_h == 2)
                    {
                        sprintf(message_text, "Will upload %s\n", k8bh.file_name);
                        output_message_to_log(log_file, message_text);
                        libcurl_upload_cimel_buffer_to_https(&k8bh, log_file);
                        free_cimel_buffer(&k8bh);
                    }
                    if (upload_switch_d == 2)
                    {
                        sprintf(message_text, "Will upload %s\n", k8bd.file_name);
                        output_message_to_log(log_file, message_text);

                        libcurl_upload_cimel_buffer_to_https(&k8bd, log_file);
                        free_cimel_buffer(&k8bd);
                    }

                    if (!if_local)
                    {
                        output_message_to_log(log_file, "Will disconnect modem\n");

                        pc_time = time(NULL);
                        system("sudo hologram network disconnect");
                        stop_time = time(NULL);
                        sprintf(message_text, "Modem disconnected after %d seconds\n", stop_time - pc_time);
                        output_message_to_log(log_file, message_text);
                    }
                }

                if (upload_switch_h)
                {
                    strcpy(k8bh.file_name, file_nameh);
                    free_cimel_buffer(&k7bh);
                }
                if (upload_switch_d)
                {
                    strcpy(k8bd.file_name, file_named);
                    free_cimel_buffer(&k8bd);
                }
                pc_time = time(NULL);
                if (!if_local)
                {
                    sprintf(message_text, "%d resets\n", reset_counter);
                    output_message_to_log(log_file, message_text);
                }

                sprintf(message_text, "Continue as model T - sleep for %d minutes %s", mcport.time_interval / 60, asctime(gmtime(&pc_time)));
                output_message_to_log(log_file, message_text);

                sleep(mcport.time_interval);
            }
        }
        else
        {
            retval = V5_main_loop_cycle(&mcport, &aerex, &k7b, log_file);

            if (retval)
            {
                if (retval == 8)
                {

                    if (mcport.cimel_number_5 == -1)
                        dev_init_5 = 1;
                    else if ((mcport.cimel_number_5 != k7b.cimel_number) || strcmp(mcport.eprom_5, k7b.eprom))
                    {
                        V5_save_k7_buffer_on_disk(backup_dir, &k7bd);
                        pc_time = time(NULL);

                        sprintf(message_text, "Will close port on %s\n", usb_port);
                        output_message_to_log(log_file, message_text);
                        close_my_port(&mcport);
                        sprintf(message_text, "Will upload existing file %s to aeronet, System clock %s\n",
                                k7bd.file_name, asctime(gmtime(&pc_time)));
                        output_message_to_log(log_file, message_text);

                        if (!if_local)
                        {
                            if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                            {
                                upload_daily_connection_log_to_ftp(log_file, username);
                                exit(0);
                            }
                        }

                        libcurl_upload_cimel_buffer_to_https(&k7bd, log_file);

                        receive_aeronet_time(&aerex, log_file);

                        if (!if_local)
                        {

                            output_message_to_log(log_file, "Will disconnect modem\n");

                            pc_time = time(NULL);
                            system("sudo hologram network disconnect");
                            stop_time = time(NULL);
                            sprintf(message_text, "Modem disconnected after %d seconds\n", stop_time - pc_time);
                            output_message_to_log(log_file, message_text);
                        }
                        free_cimel_buffer(&k7bm);
                        free_cimel_buffer(&k7bh);
                        free_cimel_buffer(&k7bd);
                        dev_init_5 = 2;

                        sprintf(message_text, "Will try to reopen port %s\n", usb_port);
                        output_message_to_log(log_file, message_text);

                        if (!define_usb_com_port(usb_port, log_file))
                        {
                            output_message_to_log(log_file, "Port unavilable. Stop the program\n");
                            exit(0);
                        }

                        strcpy(mcport.port_name, usb_port);
                        if (!open_my_com_port(&mcport, _MODEL_5_, log_file))
                        {
                            sprintf(message_text, "Port %s cannot be opened. Stop the program\n", usb_port);
                            output_message_to_log(log_file, message_text);
                            exit(0);
                        }

                        V5_wait_for_new_packet(&mcport);
                        V5_init_port_receiption(&mcport);
                    }
                    else
                        dev_init_5 = 0;

                    if (dev_init_5)
                    {
                        sprintf(message_text, "Redefined V5 Cimel number = %d  V5 eprom = %s\n", k7b.cimel_number, k7b.eprom);
                        output_message_to_log(log_file, message_text);
                        mcport.cimel_number_5 = k7b.cimel_number;
                        strcpy(mcport.eprom_5, k7b.eprom);
                        mcport.last_time_5 = 0;

                        pc_time = time(NULL);
                        gmtime_r(&pc_time, &mtim);

                        log_day1 = pc_time / 86400;
                        if (log_day1 != log_day)
                        {
                            if (!if_local)
                            {
                                if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                                {
                                    upload_daily_connection_log_to_ftp(log_file, username);
                                    exit(0);
                                }
                            }
                            upload_daily_connection_log_to_ftp(log_file, username);
                            if (!if_local)
                                system("sudo hologram network disconnect");
                            sprintf(log_file, "%s/connection_log_%d_%02d_%02d.txt",
                                    log_dir, mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
                            log_day = log_day1;
                        }

                        gmtime_r(&pc_time, &mtim);
                        sprintf(k7bm.file_name, "%s_%04d_%d%02d%02d_%02d%02d.K7", k7b.eprom, k7b.cimel_number,
                                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour, mtim.tm_min);
                        sprintf(k7bh.file_name, "%s_%04d_%d%02d%02d_%02d.K7", k7b.eprom, k7b.cimel_number,
                                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour);
                        sprintf(k7bd.file_name, "%s_%04d_%d%02d%02d.K7", k7b.eprom, k7b.cimel_number,
                                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
                    }
                }
                else
                {
                    if ((retval == 2) || (retval == 3))
                    {
                        pc_time = time(NULL);
                        gmtime_r(&pc_time, &mtim);

                        log_day1 = pc_time / 86400;
                        if (log_day1 != log_day)
                        {
                            if (!if_local)
                            {
                                if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                                {
                                    upload_daily_connection_log_to_ftp(log_file, username);
                                    exit(0);
                                }
                            }
                            upload_daily_connection_log_to_ftp(log_file, username);
                            if (!if_local)
                                system("sudo hologram network disconnect");
                            sprintf(log_file, "%s/connection_log_%d_%02d_%02d.txt",
                                    log_dir, mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
                            log_day = log_day1;
                        }
                        sprintf(message_text, "k7 buffer downloaded num = %d  %s", k7b.num_records, asctime(gmtime(&pc_time)));
                        output_message_to_log(log_file, message_text);
                        V5_save_k7_buffer_on_disk(homedir, &k7b);

                        combine_cimel_buffers(&k7bm, &k7b);
                        combine_cimel_buffers(&k7bh, &k7b);
                        combine_cimel_buffers(&k7bd, &k7b);
                        V5_save_k7_buffer_on_disk(backup_dir, &k7bd);
                        free_cimel_buffer(&k7b);
                    }

                    if ((retval == 1) || (retval == 3))
                    {

                        new_time = time(NULL);
                        gmtime_r(&new_time, &mtim);

                        log_day1 = new_time / 86400;
                        if (log_day1 != log_day)
                        {
                            if (!if_local)
                            {
                                if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                                {
                                    upload_daily_connection_log_to_ftp(log_file, username);
                                    exit(0);
                                }
                            }
                            upload_daily_connection_log_to_ftp(log_file, username);
                            if (!if_local)
                                system("sudo hologram network disconnect");
                            sprintf(log_file, "%s/connection_log_%d_%02d_%02d.txt",
                                    log_dir, mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
                            log_day = log_day1;
                        }

                        sprintf(message_text, "Interval reached system time %s", asctime(gmtime(&new_time)));
                        output_message_to_log(log_file, message_text);

                        upload_switch = upload_switch_m = upload_switch_h = upload_switch_d = 0;

                        gmtime_r(&new_time, &mtim);
                        sprintf(file_namem, "%s_%04d_%d%02d%02d_%02d%02d.K7", mcport.eprom_5, mcport.cimel_number_5,
                                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour, mtim.tm_min);
                        sprintf(file_nameh, "%s_%04d_%d%02d%02d_%02d.K7", mcport.eprom_5, mcport.cimel_number_5,
                                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour);
                        sprintf(file_named, "%s_%04d_%d%02d%02d.K7", mcport.eprom_5, mcport.cimel_number_5,
                                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);

                        if (strcmp(file_namem, k7bm.file_name))
                        {

                            if (k7bm.if_header)
                                upload_switch_m = upload_switch = 2;
                            else
                                upload_switch_m = 1;
                        }

                        if (strcmp(file_nameh, k7bh.file_name))
                        {
                            if (k7bh.if_header)
                                upload_switch_h = upload_switch = 2;
                            else
                                upload_switch_h = 1;
                        }
                        if (strcmp(file_named, k7bd.file_name))
                        {
                            if (k7bd.if_header)
                                upload_switch_d = upload_switch = 2;
                            else
                                upload_switch_d = 1;
                        }

                        if (!upload_switch)
                        {

                            sprintf(message_text, "There is nothing new to upload\nWill close port on %s\n", usb_port);
                            output_message_to_log(log_file, message_text);

                            close_my_port(&mcport);

                            sprintf(message_text, "Will try to reopen port %s\n", usb_port);
                            output_message_to_log(log_file, message_text);
                            if (!define_usb_com_port(usb_port, log_file))
                            {
                                output_message_to_log(log_file, "Port unavilable. Stop the program\n");
                                exit(0);
                            }

                            strcpy(mcport.port_name, usb_port);
                            if (!if_local)
                            {
                                sprintf(message_text, "%d resets\n", reset_counter);
                                output_message_to_log(log_file, message_text);
                            }
                            output_message_to_log(log_file, "Will check for T instrument\n");
                            if (!open_my_com_port(&mcport, _MODEL_T_, log_file))
                            {
                                output_message_to_log(log_file, "Port cannot be opened. Stop the program\n");
                                if (!if_local)
                                {
                                    if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                                    {
                                        upload_daily_connection_log_to_ftp(log_file, username);
                                        exit(0);
                                    }
                                }
                                upload_daily_connection_log_to_ftp(log_file, username);
                                if (!if_local)
                                    system("sudo hologram network disconnect");
                                exit(0);
                            }
                            if (T_receive_header_from_port(&mcport, &k8b, log_file))
                            {
                                pc_time = time(NULL);
                                cimel_model = _MODEL_T_;
                                sprintf(message_text, "Detected T instrument %s", asctime(gmtime(&pc_time)));
                                output_message_to_log(log_file, message_text);
                            }

                            else
                            {

                                pc_time = time(NULL);
                                gmtime_r(&pc_time, &mtim);

                                log_day1 = pc_time / 86400;
                                if (log_day1 != log_day)
                                {
                                    if (!if_local)
                                    {
                                        if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                                        {
                                            upload_daily_connection_log_to_ftp(log_file, username);
                                            exit(0);
                                        }
                                    }
                                    upload_daily_connection_log_to_ftp(log_file, username);
                                    if (!if_local)
                                        system("sudo hologram network disconnect");
                                    sprintf(log_file, "%s/connection_log_%d_%02d_%02d.txt",
                                            log_dir, mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
                                    log_day = log_day1;
                                }
                                sprintf(message_text, "No T instrument detected. Continue as model V5 %s", asctime(gmtime(&pc_time)));
                                output_message_to_log(log_file, message_text);
                                close_my_port(&mcport);

                                if (!open_my_com_port(&mcport, _MODEL_5_, log_file))
                                {
                                    sprintf(message_text, "Port %s cannot be opened. Stop the program\n", usb_port);
                                    output_message_to_log(log_file, message_text);
                                    if (!if_local)
                                    {
                                        if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                                        {
                                            upload_daily_connection_log_to_ftp(log_file, username);
                                            exit(0);
                                        }
                                    }
                                    upload_daily_connection_log_to_ftp(log_file, username);
                                    if (!if_local)
                                        system("sudo hologram network disconnect");
                                    exit(0);
                                }

                                V5_wait_for_new_packet(&mcport);
                                V5_init_port_receiption(&mcport);
                            }
                        }
                        else
                        {

                            sprintf(message_text, "Will close port on %s\n", usb_port);
                            output_message_to_log(log_file, message_text);
                            close_my_port(&mcport);

                            if (!if_local)
                            {
                                if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                                {
                                    upload_daily_connection_log_to_ftp(log_file, username);
                                    exit(0);
                                }
                            }

                            if (upload_switch_m == 2)
                            {
                                sprintf(message_text, "Will upload %s\n", k7bm.file_name);
                                output_message_to_log(log_file, message_text);
                                libcurl_upload_cimel_buffer_to_https(&k7bm, log_file);
                            }
                            if (upload_switch_h == 2)
                            {
                                sprintf(message_text, "Will upload %s\n", k7bh.file_name);
                                output_message_to_log(log_file, message_text);
                                libcurl_upload_cimel_buffer_to_https(&k7bh, log_file);
                            }
                            if (upload_switch_d == 2)
                            {
                                sprintf(message_text, "Will upload %s\n", k7bd.file_name);
                                output_message_to_log(log_file, message_text);
                                libcurl_upload_cimel_buffer_to_https(&k7bd, log_file);
                            }

                            receive_aeronet_time(&aerex, log_file);
                            if (!if_local)
                            {
                                output_message_to_log(log_file, "Will disconnect modem\n");

                                pc_time = time(NULL);
                                system("sudo hologram network disconnect");
                                stop_time = time(NULL);
                                sprintf(message_text, "Modem disconnected after %d seconds\n", stop_time - pc_time);
                                output_message_to_log(log_file, message_text);
                            }

                            sprintf(message_text, "Will try to reopen port %s\n", usb_port);
                            output_message_to_log(log_file, message_text);
                            if (!define_usb_com_port(usb_port, log_file))
                            {
                                printf("Port unavilable. Stop the program\n");
                                exit(0);
                            }

                            strcpy(mcport.port_name, usb_port);
                            if (!if_local)
                            {
                                sprintf(message_text, "%d resets\n", reset_counter);
                                output_message_to_log(log_file, message_text);
                            }
                            output_message_to_log(log_file, "Will check for T instrument\n");
                            if (!open_my_com_port(&mcport, _MODEL_T_, log_file))
                            {
                                output_message_to_log(log_file, "Port cannot be opened. Stop the program\n");
                                if (!if_local)
                                {
                                    if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                                    {
                                        upload_daily_connection_log_to_ftp(log_file, username);
                                        exit(0);
                                    }
                                }
                                upload_daily_connection_log_to_ftp(log_file, username);
                                if (!if_local)
                                    system("sudo hologram network disconnect");
                                exit(0);
                            }
                            if (T_receive_header_from_port(&mcport, &k8b, log_file))
                            {
                                pc_time = time(NULL);
                                gmtime_r(&pc_time, &mtim);

                                log_day1 = pc_time / 86400;
                                if (log_day1 != log_day)
                                {
                                    if (!if_local)
                                    {
                                        if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                                        {
                                            upload_daily_connection_log_to_ftp(log_file, username);
                                            exit(0);
                                        }
                                    }
                                    upload_daily_connection_log_to_ftp(log_file, username);
                                    if (!if_local)
                                        system("sudo hologram network disconnect");
                                    sprintf(log_file, "%s/connection_log_%d_%02d_%02d.txt",
                                            log_dir, mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
                                    log_day = log_day1;
                                }
                                cimel_model = _MODEL_T_;
                                sprintf(message_text, "Detected T instrument %s", asctime(gmtime(&pc_time)));
                                output_message_to_log(log_file, message_text);
                            }

                            else
                            {
                                pc_time = time(NULL);
                                gmtime_r(&pc_time, &mtim);

                                log_day1 = pc_time / 86400;
                                if (log_day1 != log_day)
                                {
                                    if (!if_local)
                                    {
                                        if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter, log_file))
                                        {
                                            upload_daily_connection_log_to_ftp(log_file, username);
                                            exit(0);
                                        }
                                    }
                                    upload_daily_connection_log_to_ftp(log_file, username);
                                    if (!if_local)
                                        system("sudo hologram network disconnect");

                                    sprintf(log_file, "%s/connection_log_%d_%02d_%02d.txt",
                                            log_dir, mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday);
                                    log_day = log_day1;
                                }
                                sprintf(message_text, "No T instrument detected. Continue as model V5 %s", asctime(gmtime(&pc_time)));
                                output_message_to_log(log_file, message_text);
                                close_my_port(&mcport);
                                if (!open_my_com_port(&mcport, _MODEL_5_, log_file))
                                {
                                    sprintf(message_text, "Port %s cannot be opened. Stop the program\n", usb_port);
                                    output_message_to_log(log_file, message_text);
                                    exit(0);
                                }

                                V5_wait_for_new_packet(&mcport);
                                V5_init_port_receiption(&mcport);
                            }
                        }
                        if (upload_switch_m)
                        {
                            free_cimel_buffer(&k7bm);
                            strcpy(k7bm.file_name, file_namem);
                        }
                        if (upload_switch_h)
                        {
                            free_cimel_buffer(&k7bh);
                            strcpy(k7bh.file_name, file_nameh);
                        }
                        if (upload_switch_d)
                        {
                            free_cimel_buffer(&k7bd);
                            strcpy(k7bd.file_name, file_named);
                        }
                    }
                }
            }
        }
    }
}
