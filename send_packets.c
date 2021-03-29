// Tomasz Woszczynski 307690
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include "send_packets.h"

uint16_t compute_icmp_checksum (const void *buff, int length)
{
	uint32_t sum;
	const uint16_t* ptr = buff;
	assert (length % 2 == 0);
	for (sum = 0; length > 0; length -= 2)
		sum += *ptr++;
	sum = (sum >> 16) + (sum & 0xffff);
	return (uint16_t)(~(sum + (sum >> 16)));
}

// Function used to send packets to given ip address (as a string), selected
// socket. pid is used for identifying packets and array seqnums is used for
// creating sequence number for each packet.
void send_packets(char *ip, int socket_fd, uint16_t pid, uint16_t seqnums[]) {
    // Set the receipent of the packet based on the arguments passed, then use
	// bzero to remove any set bits from the struct. 
	struct sockaddr_in receipent;
    bzero(&receipent, sizeof(receipent));
	
	receipent.sin_family = AF_INET; // AF_INET == IPv4, AF_INET6 == IPv6
	
	// inet_pton converts IPv4 address from text to binary, it may return
	// 0 if the input buffer pointed to by ip is not valid string or -1 if
	// the af (in our case AF_INET) does not conatin a valid address family
	if (inet_pton(AF_INET, ip, &receipent.sin_addr) <= 0) {
		fprintf(stderr, "[ERR] inet_pton: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Prepare ICMP data to be sent. As we send ICMP echo requests, there is
	// no need to put anything more than the header. We need to send three
	// packets to each IP, so our unique ID would be PID of traceroute
	// instance and only sequence number will vary. Because of that, we
	// might distinguish all packets. We use htons on pid to change endianness.
	struct icmp header;
	header.icmp_type = ICMP_ECHO;
	header.icmp_code = 0;
	header.icmp_hun.ih_idseq.icd_id = htons(pid);

	for (int i = 0; i < 3; i++) {
		// As mentioned earlier, each packet needs to be distinguishable, so it
		// contains unique sequence number and its own checksum.
		header.icmp_hun.ih_idseq.icd_seq = htons(seqnums[i]);
		header.icmp_cksum = 0;
		header.icmp_cksum = 
			compute_icmp_checksum((uint16_t*)&header, sizeof(header));

		ssize_t bytes_sent = sendto(
			socket_fd, 
			&header, 
			sizeof(header), 
			0,
			(struct sockaddr*)&receipent, 
			sizeof(receipent)
		);

		if (bytes_sent < 0) {
			fprintf(stderr, "[ERR] sendto: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}