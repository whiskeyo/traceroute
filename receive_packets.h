// Tomasz Woszczynski 307690
#ifndef RECEIVE_PACKETS_H
#define RECEIVE_PACKETS_H

#include <stdint.h>

enum answer_e {
    CORRECT,            // prints IP AVGTIME
    MISSING_ANSWER,     // prints IP ???
    NO_ANSWERS_IN_TIME, // prints *
};

// answer has higher priority than avg_repsonse_time and if it is set to
// MISSING_ANSWER or NO_ANSWERS_IN_TIME, we just skip avg_response_time,
// even if the value has changed.
struct recvdata {
    char *ips;
    enum answer_e answer;
    uint32_t avg_response_time;
};

bool is_packet_correct(struct icmp* header, uint16_t pid, uint16_t seqnums[]);
struct recvdata parse_answer(char ips[3][20], uint32_t response_times[3]);
struct recvdata receive_packets(int socket_fd, uint16_t pid, uint16_t seqnums[]);

#endif