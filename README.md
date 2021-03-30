# traceroute

## What is `traceroute` for?

`traceroute` is used for tracing IP addresses of routers on the route from the source up to the destination. It also shows average response times on every hop. This version of `traceroute` handles only IPv4 and web addresses (such like `google.com`) **can not be resolved**.

## Example usage

In order for `traceroute` to work, you need *root* privileges. It is caused because of creating raw sockets. Example call might be:

```
sudo ./traceroute 156.17.4.1
```

Possible output:

```
 1.        192.168.0.1  2 ms
 2.              * * *
 3.       172.20.253.1  12 ms
 4.        172.17.39.9  37 ms
 5.       172.17.39.10  37 ms
 6.       172.17.39.13  37 ms
 7.      172.17.28.194  ???
 8.      172.17.28.194  ???
 9.    213.249.121.205  76 ms
10.         4.69.159.5  64 ms
11.      212.162.10.82  76 ms
12.    212.191.224.106  ???
13.      156.17.254.70  ???
14.     156.17.254.105  74 ms
15.      156.17.252.37  ???
16.      156.17.252.26  75 ms
17.      156.17.252.25  74 ms
18.         156.17.4.1  78 ms
```

## Extra debugging information

You may receive extra debugging infromation when you compile `traceroute` with `make debug`. Running the program will show you socket file descriptor's number, process id of `traceroute`, and extra data about packets sent: TTL, sequence numbers, IPs of routers that answered or suitable information if it has not. Example output:

```
[DEBUG] Debug session started
[DEBUG] Socket file descriptor: 3, process id: 3601

[DEBUG] TTL: 1, sequence numbers: {1, 2, 3}
[DEBUG] Router no. 1 is 192.168.0.1, response time is 3 ms
[DEBUG] Router no. 2 is 192.168.0.1, response time is 3 ms
[DEBUG] Router no. 3 is 192.168.0.1, response time is 3 ms
[DEBUG] Correctly received info from all routers
[ROUTE]  1.        192.168.0.1  3 ms

[DEBUG] TTL: 2, sequence numbers: {4, 5, 6}
[DEBUG] Router no. 1 is missing an answer
[DEBUG] Router no. 2 is missing an answer
[DEBUG] Router no. 3 is missing an answer
[DEBUG] None of the routers answered, IP unknown
[ROUTE]  2.              * * *

[DEBUG] TTL: 3, sequence numbers: {7, 8, 9}
[DEBUG] Router no. 1 is 172.20.253.1, response time is 10 ms
[DEBUG] Router no. 2 is 172.20.253.1, response time is 10 ms
[DEBUG] Router no. 3 is 172.20.253.1, response time is 18 ms
[DEBUG] Correctly received info from all routers
[ROUTE]  3.       172.20.253.1  12 ms

[DEBUG] TTL: 4, sequence numbers: {10, 11, 12}
[DEBUG] Router no. 1 is 172.17.39.9, response time is 36 ms
[DEBUG] Router no. 2 is 172.17.39.9, response time is 36 ms
[DEBUG] Router no. 3 is missing an answer
[DEBUG] One of routers has not answered or timed out
[ROUTE]  4.        172.17.39.9  ???
```

## How does it work?

Raw socket is created with the ICMP protocol. As we want to trace all routers from source IP to destination IP, we need to limit the distance the packet can pass. In order to do that, value of TTL is set to 1 at the beginning, and with further routers it is incremented up to 30 (getting into router lowers TTL by 1). It is very imporant to differ all sent packets from possible many instances of `traceroute`, thus in every packet this data is stored:

* process id of `traceroute` instance
* sequence number of the packet (based on TTL)

Thanks to these two values, all packets from all instances of `traceroute` are easily distinguishable and they can not mix with each other.

When it comes to sending packets, all of them are ICMP *echo requests*. Each of these packets has its own *pid*, *seqnum* and the checksum. On each hop, three packets are being sent.

Receiving packets is based on "catching" correct packets in each hop. There are two types of packets that are interesting: ICMP *echo reply* and ICMP *time limit exceeded*. The latter holds the header of the sent packet, so when this type of packet is encountered, correct header needs to be extracted. Of course *pid* and *seqnums* needs to be correct, otherwise it means that the received packet does not belong to the `traceroute`.

Received packets can come from various routers, which means that there are at most 3 different IPs on the hop. When all three packets are caught, average response time is calculated (in *ms*) and the output is `[ip] [avg] ms`, when at least one of them is missing, the output is `[ip] ???` (there is no point of calculating average when one packet is lost), and when all packets are missing, `***` is printed.