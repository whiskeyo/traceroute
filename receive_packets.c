// Tomasz Woszczynski 307690
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include "receive_packets.h"

bool is_packet_correct(struct icmp* header, uint16_t pid, uint16_t seqnums[]) {
    // In order for a received packet to be correct, these requirements need
    // to be met:
    // * ICMP type has to be either Echo Reply or Time Exceeded
    // * pid of traceroute instance has to be the same as in sent packet
    // * sequence number needs to be the same as one of sequence numbers
    //   sent in one of three packets sent

    int type = header->icmp_type;
    if (type != ICMP_ECHOREPLY && type != ICMP_TIME_EXCEEDED)
        return false;

    // If TTL exceeded, we get a response a packet with TTL exceeded information
    // and the copy of what was sent (echo reply) and this is what is interesting 
    // to us. We need to "parse" the response and overwrite TTL exceeded header 
    // with the Echo Reply header.
    if (type == ICMP_TIME_EXCEEDED) {
        struct ip *ip_echo_header = 
            (struct ip*)((void*)header + (ssize_t)sizeof(struct icmphdr));

        header = (struct icmp*)((void*)ip_echo_header + 4 * ip_echo_header->ip_hl);
    }

    if (ntohs(header->icmp_hun.ih_idseq.icd_id) != pid)
        return false;

    int16_t seqnum = ntohs(header->icmp_hun.ih_idseq.icd_seq);
    if (seqnum != seqnums[0] && seqnum != seqnums[1] && seqnum != seqnums[2])
        return false;

    return true;
}

void append_ips(char *dst, const char *src, int i) {    
    if (i == 0)
        strcpy(dst, src);
    else {
        strcat(dst, " ");
        strcat(dst, src);
    }
}

// Based on the received data of packets, calculate the average response times,
// save all routers' IPs from the hop and based on these values, set the value
// of enum answer_e (which is then used for printing output). 
struct recvdata parse_answer(char ips[3][20], uint32_t response_times[3]) {
    char ips_str[60];
    uint32_t avg_response_time = 0;

    bool missing_answer = false;
    bool no_ips_received = true;

    for (int i = 0; i < 3; i++) { 
        if (response_times[i] == 0) {
            // If there is response time equal to 0 ms, it means that the
            // answer is missing (probably by timing out).
            missing_answer = true;
#ifdef DEBUG
    fprintf(stderr, DEBUGLOG " Router no. %d is missing an answer\n", i + 1);
#endif
        } else {
            // Otherwise we got some answer from the router. It means that the
            // IP address is now known and we should add it to the route. Of
            // course if it is the first IP, we need to add it, when it is 
            // from second or third router we need to append it to the list of
            // IPs only when it is different from earlier routers on the hop.
            no_ips_received = false;

#ifdef DEBUG
    fprintf(stderr, DEBUGLOG " Router no. %d is %s\n", i + 1, ips[i]);
#endif

            if (i == 0)
                append_ips(ips_str, ips[0], 0);
            else if (i == 1 && ips[1][0] != '\0' && (strcmp(ips[0], ips[1]) != 0)) 
                append_ips(ips_str, ips[0], 1);
            else if (ips[2][0] != '\0' &&
                    (strcmp(ips[0], ips[2]) != 0 || strcmp(ips[1], ips[2]) != 0)) 
                append_ips(ips_str, ips[2], 2);   
        }

        avg_response_time += response_times[i];
    }

    enum answer_e answer;
    if (no_ips_received) 
        answer = NO_ANSWERS_IN_TIME;
    else if (missing_answer) 
        answer = MISSING_ANSWER;
    else {
        answer = CORRECT;
        avg_response_time /= 3;
    }

#ifdef DEBUG
    char* ans;
    switch (answer) {
        case CORRECT: 
            ans = "Correctly received info from all routers"; 
            break;
        case MISSING_ANSWER:
            ans = "One of routers has not answered or timed out";
            break;
        case NO_ANSWERS_IN_TIME:
            ans = "None of the routers answered, IP unknown";
            break;
    }

    fprintf(stderr, DEBUGLOG " %s\n", ans);
#endif

    struct recvdata parsed = { ips_str, answer, avg_response_time };
    return parsed;
}

struct recvdata receive_packets(int socket_fd, uint16_t pid, uint16_t seqnums[]) {
    // We want to wait at most 1 second in socket_fd for the packet. Of course
    // we are waiting for 3 packets from each hop (1 hop = 1 TTL).
    fd_set descriptors;
    FD_ZERO(&descriptors);
    FD_SET(socket_fd, &descriptors);
    
    struct timeval time;
    time.tv_sec  = 1;
    time.tv_usec = 0; 

    char sender_ip_str[20];
    char ips[3][20] = { "", "", "" };
    uint32_t response_times[3] = { };

    for (int i = 0; i < 3; i++) {
        struct sockaddr_in sender;
        socklen_t          sender_len = sizeof(sender);
        uint8_t            buffer[IP_MAXPACKET];

        int ready = select(socket_fd + 1, &descriptors, NULL, NULL, &time);
        if (ready < 0) {
            fprintf(stderr, "[ERR] select: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (ready == 0) // timeout after x seconds, saved in time 
            break;

        ssize_t packet_len = recvfrom(
            socket_fd, 
            buffer, 
            IP_MAXPACKET, 
            0,
            (struct sockaddr*)&sender, 
            &sender_len
        );

        if (packet_len < 0) {
            fprintf(stderr, "[ERR] recvfrom: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // If packet was successfully received, convert it from binary to text.
        // Similar thing happends in send_packets, but it converts from text to bin.
        if (inet_ntop(AF_INET, &(sender.sin_addr), sender_ip_str, sizeof(sender_ip_str)) == NULL) {
            fprintf(stderr, "[ERR] inet_ntop: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        struct ip   *ip_header   = (struct ip*) buffer;
        struct icmp *icmp_header = (struct icmp*) (buffer + 4 * ip_header->ip_hl);

        // If we receive incorrect packet (wrong type or seqnum does not belong
        // to any of three packets), we repeat waiting for the packet. It means
        // that we always need to collect three packets.
        if (!is_packet_correct(icmp_header, pid, seqnums)) {
            i--;
            continue;
        }

        strcpy(ips[i], sender_ip_str);

        // We count time in the range [0,1) and the basic unit is microsecond,
        // thus we divide it by 1000 to get time in milliseconds.
        response_times[i] = (1000000 - time.tv_usec) / 1000;
    }

    return parse_answer(ips, response_times);
}