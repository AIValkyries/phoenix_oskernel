#ifndef _DEV_H
#define _DEV_H

#include <eth.h>
#include <ip.h>
#include <tcp.h>
#include <sock.h>

#define MAX_HEADER  sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct tcphdr)   // eth+ip+tcp

#endif