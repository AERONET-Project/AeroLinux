#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <dirent.h>
#include <sys/stat.h>

#include <curl/curl.h>

#include "models_port.h"

int filter_k8_and_k7_files(const struct dirent *dp)
{

    int lentt = strlen(dp->d_name);

    if (lentt < 4)
        return 0;
    if (lentt > 100)
        return 0;

    if (dp->d_name[lentt - 3] != '.')
        return 0;
    if ((dp->d_name[lentt - 2] != 'k') && (dp->d_name[lentt - 2] != 'K'))
        return 0;
    if ((dp->d_name[lentt - 1] != '7') && (dp->d_name[lentt - 1] != '8'))
        return 0;

    return 1;
}

int main(int argc, char **argv)
{

    struct dirent **list_of_files, *dp;
    time_t now = time(NULL), read_time;
    int num_days, day1, day2, num_files, ifile, day_num;

    char *homedir = getenv("HOME"), backup_dir[300], file_name[400];
    struct stat bufff;
    CIMEL_BUFFER cimel_buffer;

    sprintf(backup_dir, "%s/backup", homedir);

    day2 = now / 86400;
    day1 = 0;
    if (argc > 1)
    {
        num_days = atoi(argv[1]);
        if (num_days)
        {
            day1 = day2 - num_days;
        }
    }

    // find files on the backup directory $HOME/backup

    num_files = scandir(backup_dir, &list_of_files, &filter_k8_and_k7_files, NULL);

    for (ifile = 0; ifile < num_files; ifile++)
    {
        dp = list_of_files[ifile];
        sprintf(file_name, "%s/%s", backup_dir, dp->d_name);

        if (!stat(file_name, &bufff))
        {
            day_num = bufff.st_mtime / 86400;

            if ((day_num >= day1) && (day_num < day2))
            {

                printf("Found file %s\n", dp->d_name);
                strcpy(cimel_buffer.file_name, dp->d_name);

                if (dp->d_name[strlen(dp->d_name) - 1] == '7')
                    read_time = V5_read_k7_buffer_from_disk(backup_dir, &cimel_buffer);
                else
                    read_time = T_read_k8_buffer_from_disk(backup_dir, &cimel_buffer);

                if (read_time)
                {
                    libcurl_upload_cimel_buffer_to_https(&cimel_buffer, NULL);
                    free_cimel_buffer(&cimel_buffer);
                }
            }
        }
    }
}
