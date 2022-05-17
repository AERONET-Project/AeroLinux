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

#include <curl/curl.h>

typedef struct
{
    time_t aeronet_time;
    char pc_time_string[20];
    int aeronet_time_real, seconds_shift, status, good_clock;
} AERO_EXCHANGE;

size_t dataSize;

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

time_t receive_aeronet_time(AERO_EXCHANGE *ptr, char *buffer, char *file_name)
{
    CURL *curl;
    CURLcode res;
    time_t cr_time;
    struct tm mtim;

    time_t pi_time;

    char *kurka = buffer, hostname[100], *username;

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

        gethostname(hostname, 99);
        username = getenv("USER");

        sprintf(kurka, "PI_HOST_NAME=%s\nUSER=%s\n", hostname, username);
        kurka += strlen(kurka);

        gmtime_r(&pi_time, &mtim);
        sprintf(kurka, "PI_Time=%02d:%02d:%d,%02d:%02d:%02d\n", mtim.tm_mday, mtim.tm_mon + 1, mtim.tm_year + 1900,
                mtim.tm_hour, mtim.tm_min, mtim.tm_sec);
        kurka += strlen(kurka);

        sprintf(file_name, "%s_%02d-%02d-%d_%02d-%02d-%02d", username, mtim.tm_mday, mtim.tm_mon + 1, mtim.tm_year + 1900,
                mtim.tm_hour, mtim.tm_min, mtim.tm_sec);

        gmtime_r(&ptr->aeronet_time, &mtim);
        sprintf(kurka, "Aeronet_Time=%02d:%02d:%d,%02d:%02d:%02d\n", mtim.tm_mday, mtim.tm_mon + 1, mtim.tm_year + 1900,
                mtim.tm_hour, mtim.tm_min, mtim.tm_sec);
        kurka += strlen(kurka);

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
        return 0;
    }

    printf("\n");

    return ptr->aeronet_time;
}

size_t upload_hb_buffer_internally(unsigned char *buffer, size_t size, size_t nmemb, unsigned char *hb_buffer)
{

    size_t total = size * nmemb;

    if (total > dataSize)
        total = dataSize;

    if (total > 0)
    {

        memcpy(buffer, hb_buffer, total);
        memcpy(hb_buffer, hb_buffer + total, dataSize - total);
        dataSize -= total;
    }
    return total;
}

int upload_hb_buffer_to_ftp(unsigned char *hb_buffer, char *upload_name)
{
    char *bufname = getlogin();
    CURL *curl;
    CURLcode res;
    char sscctt[200];
    int i;

    printf("Will upload file %s with %d bytes\n", upload_name, dataSize);

    curl = curl_easy_init();

    if (!curl)
        return 0;

    sprintf(sscctt, "ftp://aeronet.gsfc.nasa.gov/pub/incoming/raspberry_pi_heartbeat/%s/%s",bufname, upload_name);

    curl_easy_setopt(curl, CURLOPT_URL, sscctt);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
    curl_easy_setopt(curl, CURLOPT_USERPWD, "anonymous:me@email.net");

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, upload_hb_buffer_internally);
    curl_easy_setopt(curl, CURLOPT_READDATA, hb_buffer);
    printf("Will perform curl\n");
    res = curl_easy_perform(curl);
    printf("Curlopt return %d\n", res);
    curl_easy_cleanup(curl);
    if (res == CURLE_OK)
        return 1;

    return 0;
}

int main(int argc, char **argv)
{

    time_t local_time, aeronet_time;
    char heartbeat_buffer[500], file_name[300], *kurka;

    AERO_EXCHANGE aerex;
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    local_time = time(NULL);
    if (receive_aeronet_time(&aerex, heartbeat_buffer, file_name))
    {

        kurka = heartbeat_buffer + strlen(heartbeat_buffer);

        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s == 0)
            {
                sprintf(kurka, "<Interface>: %s   <Address> %s\n", ifa->ifa_name, host);
                kurka += strlen(kurka);
            }
        }
        dataSize = strlen(heartbeat_buffer);

        printf("Formed buffer to upload:\n%s\nSize = %d\n", heartbeat_buffer, dataSize);
        upload_hb_buffer_to_ftp(heartbeat_buffer, file_name);
    }
}
