# traceroute

## What is `traceroute` for?

`traceroute` is used for tracing IP addresses of routers on the route from the source up to the destination. It also shows average response times on every hop. This version of `traceroute` handles only IPv4 and web addresses (such like `google.com`) **can not be resolved**. 

## Example usage

In order for `traceroute` to work, you need *root* privileges. It is caused because of creating raw sockets. Example call might be:

```
sudo ./traceroute 156.17.254.113
```

Possible output:

```
 1.          192.168.0.1        2 ms
 2.                * * *
 3.         172.20.253.1        9 ms
 4.          172.17.39.9        22 ms
 5.         172.17.39.10        19 ms
 6.         172.17.39.13        20 ms
 7.        172.17.28.194        25 ms
 8.        172.17.28.194        27 ms
 9.      213.249.121.205        57 ms
10.           4.69.159.5        51 ms
11.        212.162.10.82        53 ms
12.      212.191.224.106        56 ms
13.       156.17.254.113        55 ms
```

## How does it work?

Raw socket is created with the ICMP protocol. As we want to trace all routers from source IP to destination IP, we need to limit the distance the packet can pass. In order to do that, value of TTL is set to 1 at the beginning, and with further routers it is incremented up to 30 (getting into router lowers TTL by 1). It is very imporant to differ all sent packets from possible many instances of `traceroute`, thus in every packet this data is stored:

* process id of `traceroute` instance
* sequence number of the packet (based on TTL)

Thanks to these two values, all packets from all instances of `traceroute` are easily distinguishable and they can not mix with each other.

When it comes to sending packets, all of them are ICMP *echo requests*. Each of these packets has its own *pid*, *seqnum* and the checksum. On each hop, three packets are being sent.

Receiving packets is based on "catching" correct packets in each hop. There are two types of packets that are interesting: ICMP *echo reply* and ICMP *time limit exceeded*. The latter holds the header of the sent packet, so when this type of packet is encountered, correct header needs to be extracted. Of course *pid* and *seqnums* needs to be correct, otherwise it means that the received packet does not belong to the `traceroute`.

Received packets can come from various routers, which means that there are at most 3 different IPs on the hop. When all three packets are caught, average response time is calculated (in *ms*) and the output is `[ip] [avg] ms`, when at least one of them is missing, the output is `[ip] ???` (there is no point of calculating average when one packet is lost), and when all packets are missing, `***` is printed.