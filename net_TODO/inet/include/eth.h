#ifndef _ETH_H
#define _ETH_H


#define ETH_ALEN    6


struct ethhdr
{
    unsigned char h_dest[ETH_ALEN];
    unsigned char h_source[ETH_ALEN];
    unsigned short h_proto;         // packet type id field
};




#endif