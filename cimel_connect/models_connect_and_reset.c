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
    char backup_dir[500], *homedir = getenv("HOME");
    char file_nameh[400], file_named[400], file_namem[400];

    int upload_switch, upload_switch_m, upload_switch_d, upload_switch_h;
    FILE *rd;

    /*
V5 variables
*/
    CIMEL_BUFFER k7b, k7bm, k7bh, k7bd;
    struct tm mtim;
    time_t pc_time, new_time, stop_time;
    time_t last_time_5;
    AERO_EXCHANGE aerex;
    /*
T variables
*/
    CIMEL_BUFFER k8b, k8bh, k8bd;
    time_t last_time_T;

    /*
Step 0.  decide if local
*/
    // form usb reset command

    if (define_usb_reset_command(stdin, usb_reset_command))
    {
        printf("Use hologram mode, usb reset command = [%s]\n", usb_reset_command);
        if_local = 0;
    }
    else
    {
        printf("Use local mode\n");
        if_local = 1;
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

    //printf("argc = %d\n", argc);

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
        printf("Found last saved time of V5 : %sPreviously used V5 Cimel number = %d, EPROM = %s\n",
               ctime(&last_time_5), k7b.cimel_number, k7b.eprom);
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
        printf("last_time.k7 not found. No previously used V5 instrument detected\n");
        mcport.cimel_number_5 = -1;
        mcport.last_time_5 = 0;
        init_cimel_buffer(&k7bm);
        init_cimel_buffer(&k7bh);
        init_cimel_buffer(&k7bd);
        init_cimel_buffer(&k7b);
    }

    strcpy(k8b.file_name, "last_time.k8");
    printf("Now will read last_time.k8 file\n");
    last_time_T = T_read_k8_buffer_from_disk(homedir, &k8b);

    if (last_time_T)
    {
        printf("Found last saved time of T : %sPreviously used T Cimel number = %d EPROM = %s\n",
               ctime(&last_time_T), k8b.cimel_number, k8b.eprom);
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
        printf("last_time.k8 not found. No previously used T instrument detected\n");

        mcport.cimel_number_T = -1;
        mcport.last_time_T = 0;

        init_cimel_buffer(&k8bh);
        init_cimel_buffer(&k8bd);
    }

    /*
Step 4.
start main loop
*/

    printf("Initially check for T instrument at the port\n");
    cimel_model = _MODEL_T_;

    while (1)
    {

        if (cimel_model == _MODEL_T_)
        {
            pc_time = time(NULL);
            gmtime_r(&pc_time, &mtim);

            if (!mcport.if_T_port_already_open)
            {
                init_cimel_buffer(&k8b);

                if (!define_usb_com_port(usb_port))
                {
                    printf("No Serial - USB ports are detected\nProgram %s stop\n", argv[0]);
                    exit(0);
                }
                strcpy(mcport.port_name, usb_port);

                if (!open_my_com_port(&mcport, _MODEL_T_))
                {
                    printf("Port cannot be opened. Stop the program\n");
                    exit(0);
                }

                if (!T_receive_header_from_port(&mcport, &k8b))
                {
                    if (!if_local)
                        printf("%d resets\n", reset_counter);

                    pc_time = time(NULL);
                    printf("T instrument appears disconnected\nWill assume V5 and switch to waiting mode %s", ctime(&pc_time));
                    close_my_port(&mcport);
                    cimel_model = _MODEL_5_;
                    if (!open_my_com_port(&mcport, _MODEL_5_))
                    {
                        printf("Port cannot be opened. Stop the program\n");
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
                        if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter))
                            exit(0);
                    }

                    libcurl_upload_cimel_buffer_to_https(&k8bd);

                    if (!if_local)
                    {

                        printf("Will disconnect modem\n");

                        pc_time = time(NULL);
                        system("sudo hologram network disconnect");
                        stop_time = time(NULL);
                        printf("Modem disconnected after %d seconds\n", stop_time - pc_time);
                    }

                    free_cimel_buffer(&k8bh);
                    free_cimel_buffer(&k8bd);
                    dev_init_T = 2;
                }
                else
                    dev_init_T = 0;

                if (dev_init_T)
                {
                    printf("Redefined T Cimel number = %d  eprom = %s\n", k8b.cimel_number, k8b.eprom);
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
                printf("Will try to retreive K8 buffer %s", ctime(&pc_time));

                if (T_retrieve_k8_buffer_data_only(&mcport, &k8b, 17000))
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
                        if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter))
                            exit(0);
                    }

                    if (upload_switch_m == 2)
                    {
                        sprintf(k8b.file_name, "%s_%04d_%d%02d%02d_%02d%02d.K8", k8b.eprom, k8b.cimel_number,
                                mtim.tm_year + 1900, mtim.tm_mon + 1, mtim.tm_mday, mtim.tm_hour, mtim.tm_min);

                        printf("Will upload %s\n", k8b.file_name);

                        libcurl_upload_cimel_buffer_to_https(&k8b);

                        strcpy(k8b.file_name, "last_time.k8");
                        free_cimel_buffer(&k8b);
                    }
                    if (upload_switch_h == 2)
                    {
                        printf("Will upload %s\n", k8bh.file_name);
                        libcurl_upload_cimel_buffer_to_https(&k8bh);
                        free_cimel_buffer(&k8bh);
                    }
                    if (upload_switch_d == 2)
                    {
                        printf("Will upload %s\n", k8bd.file_name);
                        libcurl_upload_cimel_buffer_to_https(&k8bd);
                        free_cimel_buffer(&k8bd);
                    }

                    if (!if_local)
                    {
                        printf("Will disconnect modem\n");

                        pc_time = time(NULL);
                        system("sudo hologram network disconnect");
                        stop_time = time(NULL);
                        printf("Modem disconnected after %d seconds\n", stop_time - pc_time);
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
                    printf("%d resets\n", reset_counter);
                printf("Continue as model T - sleep for %d minutes %s", mcport.time_interval / 60, ctime(&pc_time));

                sleep(mcport.time_interval);
            }
        }
        else
        {
            retval = V5_main_loop_cycle(&mcport, &aerex, &k7b);

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

                        printf("Will close port on %s\n", usb_port);
                        close_my_port(&mcport);
                        printf("Will upload existing file %s to aeronet, System clock %s\n",
                               k7bd.file_name, ctime(&pc_time));

                        if (!if_local)
                        {
                            if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter))
                                exit(0);
                        }

                        libcurl_upload_cimel_buffer_to_https(&k7bd);

                        receive_aeronet_time(&aerex);

                        if (!if_local)
                        {

                            printf("Will disconnect modem\n");

                            pc_time = time(NULL);
                            system("sudo hologram network disconnect");
                            stop_time = time(NULL);
                            printf("Modem disconnected after %d seconds\n", stop_time - pc_time);
                        }
                        free_cimel_buffer(&k7bm);
                        free_cimel_buffer(&k7bh);
                        free_cimel_buffer(&k7bd);
                        dev_init_5 = 2;

                        printf("Will try to reopen port %s\n", usb_port);
                        if (!define_usb_com_port(usb_port))
                        {
                            printf("Port unavilable. Stop the program\n");
                            exit(0);
                        }

                        strcpy(mcport.port_name, usb_port);
                        if (!open_my_com_port(&mcport, _MODEL_5_))
                        {
                            printf("Port %s cannot be opened. Stop the program\n", usb_port);
                            exit(0);
                        }
                        V5_wait_for_new_packet(&mcport);
                        V5_init_port_receiption(&mcport);
                    }
                    else
                        dev_init_5 = 0;

                    if (dev_init_5)
                    {
                        printf("Redefined V5 Cimel number = %d  V5 eprom = %s\n", k7b.cimel_number, k7b.eprom);
                        mcport.cimel_number_5 = k7b.cimel_number;
                        strcpy(mcport.eprom_5, k7b.eprom);
                        mcport.last_time_5 = 0;

                        pc_time = time(NULL);

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
                        printf("k7 buffer downloaded num = %d  %s", k7b.num_records, ctime(&pc_time));
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
                        printf("Interval reached system time %s", ctime(&new_time));

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

                            printf("There is nothing new to upload\n");
                            printf("Will close port on %s\n", usb_port);
                            close_my_port(&mcport);

                            printf("Will try to reopen port %s\n", usb_port);
                            if (!define_usb_com_port(usb_port))
                            {
                                printf("Port unavilable. Stop the program\n");
                                exit(0);
                            }

                            strcpy(mcport.port_name, usb_port);
                            if (!if_local)
                                printf("%d resets\n", reset_counter);

                            printf("Will check for T instrument\n");
                            if (!open_my_com_port(&mcport, _MODEL_T_))
                            {
                                printf("Port cannot be opened. Stop the program\n");
                                exit(0);
                            }
                            if (T_receive_header_from_port(&mcport, &k8b))
                            {
                                pc_time = time(NULL);
                                cimel_model = _MODEL_T_;
                                printf("Detected T instrument %s", ctime(&pc_time));
                            }

                            else
                            {

                                pc_time = time(NULL);
                                printf("No T instrument detected. Continue as model V5 %s", ctime(&pc_time));
                                close_my_port(&mcport);

                                if (!open_my_com_port(&mcport, _MODEL_5_))
                                {
                                    printf("Port %s cannot be opened. Stop the program\n", usb_port);
                                    exit(0);
                                }

                                V5_wait_for_new_packet(&mcport);
                                V5_init_port_receiption(&mcport);
                            }
                        }
                        else
                        {

                            printf("Will close port on %s\n", usb_port);
                            close_my_port(&mcport);

                            if (!if_local)
                            {
                                if (!connect_hologram_model_and_reset_if_error(usb_reset_command, &reset_counter))
                                    exit(0);
                            }

                            if (upload_switch_m == 2)
                            {
                                printf("Will upload %s\n", k7bm.file_name);
                                libcurl_upload_cimel_buffer_to_https(&k7bm);
                            }
                            if (upload_switch_h == 2)
                            {
                                printf("Will upload %s\n", k7bh.file_name);
                                libcurl_upload_cimel_buffer_to_https(&k7bh);
                            }
                            if (upload_switch_d == 2)
                            {
                                printf("Will upload %s\n", k7bd.file_name);
                                libcurl_upload_cimel_buffer_to_https(&k7bd);
                            }

                            receive_aeronet_time(&aerex);
                            if (!if_local)
                            {
                                printf("Will disconnect modem\n");

                                pc_time = time(NULL);
                                system("sudo hologram network disconnect");
                                stop_time = time(NULL);
                                printf("Modem disconnected after %d seconds\n", stop_time - pc_time);
                            }

                            printf("Will try to reopen port %s\n", usb_port);
                            if (!define_usb_com_port(usb_port))
                            {
                                printf("Port unavilable. Stop the program\n");
                                exit(0);
                            }

                            strcpy(mcport.port_name, usb_port);
                            if (!if_local)
                                printf("%d resets\n", reset_counter);

                            printf("Will check for T instrument\n");
                            if (!open_my_com_port(&mcport, _MODEL_T_))
                            {
                                printf("Port cannot be opened. Stop the program\n");
                                exit(0);
                            }
                            if (T_receive_header_from_port(&mcport, &k8b))
                            {
                                pc_time = time(NULL);
                                cimel_model = _MODEL_T_;
                                printf("Detected T instrument %s", ctime(&pc_time));
                            }

                            else
                            {
                                pc_time = time(NULL);
                                printf("No T instrument detected. Continue as model V5 %s", ctime(&pc_time));
                                close_my_port(&mcport);
                                if (!open_my_com_port(&mcport, _MODEL_5_))
                                {
                                    printf("Port %s cannot be opened. Stop the program\n", usb_port);
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
