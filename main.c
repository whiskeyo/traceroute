// Tomasz Woszczynski 307690
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <stdbool.h>

#include "send_packets.h"
#include "receive_packets.h"
#include "helpers.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[ERR] Invalid call! Usage: %s [IP]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!is_valid_ip_address(argv[1])) {
        fprintf(stderr, "[ERR] Passed argument is not valid IPv4 address.\n");
        return EXIT_FAILURE;
    }

    // Create raw socket with the ICMP protocol. It may fail and return -1,
    // if successful, returns file descriptor of new socket.
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        fprintf(stderr, "[ERR] socket: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    uint16_t pid = getpid();

    for (int TTL = 1; TTL <= 30; TTL++) {
        // Set socket options. We want to set TTL field for every packet, thus
        // we pass IP_TTL and its value to the socket. Thanks to it, we will
        // go through all routers on the route and collect information we need.
        if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &TTL, sizeof(int)) < 0) {
            fprintf(stderr, "[ERR] setsockopt: %s\n", strerror(errno));
            return EXIT_FAILURE;   
        }

        // Based on TTL value, sequence number is generated. It needs to be
        // used, as we need to know where packets belong (each hop has three
        // packets sent, we want to identify each of them).
        uint16_t seqnums[3] = { 3 * TTL, 3 * TTL + 1, 3 * TTL + 2};
        
        send_packets(argv[1], sockfd, pid, seqnums);
        struct recvdata response = receive_packets(sockfd, pid, seqnums);

        switch (response.answer) {
            case CORRECT: 
                printf("%2d. %20s\t%d ms\n", 
                    TTL, response.ips, response.avg_response_time);
                break;
            case MISSING_ANSWER:
                printf("%2d. %20s ???\n", TTL, response.ips);
                break;
            case NO_ANSWERS_IN_TIME:
                printf("%2d. %20s\n", TTL, "* * *");
                break;
        }

        // Response is equal to passed argument, so we reached our destination.
        if (strcmp(response.ips, argv[1]) == 0)
            break;
    }

    // At the end close the file descriptor, as we do not want it to be opened
    // unnecessarily. Just good programming practice :)
    if (close(sockfd) < 0) {
        fprintf(stderr, "[ERR] close: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}