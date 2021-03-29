// Tomasz Woszczynski 307690
#include <arpa/inet.h>
#include <stdbool.h>
#include "helpers.h"

// inet_pton converts IP from text to binary, if it returns 1 it means that
// conversion was successful and given IP is indeed a valid address. Used to
// check whether passed argument to traceroute is correct. 
bool is_valid_ip_address(char *ip) {
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip, &(sa.sin_addr));
    return result == 1;
}