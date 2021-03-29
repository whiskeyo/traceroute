// Tomasz Woszczynski 307690
#ifndef SEND_PACKETS_H
#define SEND_PACKETS_H

#include <stdint.h>

uint16_t compute_icmp_checksum (const void *buff, int length);
void send_packets(char *ip, int socket_fd, uint16_t pid, uint16_t seqnums[]);

#endif