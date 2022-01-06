#include "inc.h"
#include "network.h"


void got_packet(u_char *argv, const struct pcap_pkthdr *header, const u_char *packet) {}

int main(int argc, char **argv) {
    char errbuf[PCAP_ERRBUF_SIZE]; /* Error string */
   
    struct pcap_pkthdr header;     /* The header that "pcap gives us", not the packet itself*/
    const u_char *packet;          /* The actual packet */
    bpf_u_int32 mask;              /* The netmask of our sniffing device */
    bpf_u_int32 net;               /* The IP of our sniffing device */
    int optimize;                  /* The optimize flag while compiling.*/
    struct ether_header *eptr;
    int packet_cnt = 0;

    int ret;

    memset(errbuf, '\0', PCAP_ERRBUF_SIZE * sizeof(char));
    char *filter_exp = strdup("");
    optimize = 1;
    mask = 0;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <filename or - for stdin>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pcap_t *handle = pcap_open_offline(argv[1], errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open file %s: %s\n", argv[1], errbuf);
        return EXIT_FAILURE;
    }
    struct bpf_program fp;
    ret = pcap_compile(handle, &fp, filter_exp, optimize,
                       mask);  // enable optmization by setting flag = 1
    if (ret == PCAP_ERROR) {
        fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return EXIT_FAILURE;
    }
    ret = pcap_setfilter(handle, &fp);
    if (ret == PCAP_ERROR) {
        fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return EXIT_FAILURE;
    }

    packet_cnt = 0;
    /* Grab a packet */
    while ((packet = pcap_next(handle, &header)) !=
           NULL) {  // packet = the start pos. of the packet
        /* Print its length */
        puts("");
        printf("#%d\n", ++packet_cnt);
        printf("len of this packet: %u\n", header.len);
        printf("len of portion present: %u\n", header.caplen);
        printf("timestamp: %u\n", header.ts.tv_sec * 1000000 + header.ts.tv_usec);

        eptr = (struct ether_header *)packet;
        printf("dest MAC address: ");
        for (int i = 0; i < 6; ++i) {
            printf("%02x%c", eptr->ether_dhost[i], ":\n"[i == 5]);
        }
        printf("src MAC address: ");
        for (int i = 0; i < 6; ++i) {
            printf("%02x%c", eptr->ether_shost[i], ":\n"[i == 5]);
        }

        // https://en.wikipedia.org/wiki/EtherType
        switch (ntohs(eptr->ether_type)) {
            case ETHERTYPE_IP:
                printf("Ethertype: IPv4\n");
                parse_ipv4(packet + 14, header.len - 14);  // MAC: 6 bytes, Type: 2 bytes
                break;
            case ETHERTYPE_IPV6:
                printf("Ethertype: IPv6\n");
                parse_ipv6(packet + 14, header.len - 14);
                break;
            case ETHERTYPE_IPX:
                printf("Ethertype: IPX\n");
                break;
            case ETHERTYPE_ARP:
                printf("Ethertype: ARP\n");
                break;
            case 0x0842:
                printf("Ethertype: WoL\n");
                break;
            case ETHERTYPE_REVARP:
                printf("Ethertype: RARP\n");
                break;

            default:
                printf("unknown type: %04x\n", ntohs(eptr->ether_type));
                puts("please google yourself.");
                break;
        }
        printf("\n---------------------------------------------------\n");
    }
    /* And close the session */
    pcap_close(handle);
    // ret = pcap_loop(handle, 1, got_packet, NULL);
    return EXIT_SUCCESS;
}