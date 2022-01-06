#include "parse.h"

#include "inc.h"

u_char *parse_udp(const u_char *start, int len) {
    struct udphdr *uptr;

    uptr = (struct udphdr *)start;
    printf("src port: %u\n", ntohs(uptr->uh_sport));
    printf("dest port: %u\n", ntohs(uptr->uh_dport));
    printf("Segment length: %u\n", ntohs(uptr->uh_ulen));
}

u_char *parse_tcp(const u_char *start, int len) {
    struct tcphdr *tptr;

    tptr = (struct tcphdr *)start;
    printf("src port: %u\n", ntohs(tptr->th_sport));
    printf("dest port: %u\n", ntohs(tptr->th_dport));
    printf("seq num: %u\n", ntohl(tptr->th_seq));
    printf("acknowledgment num: %u\n", ntohl(tptr->th_ack));
    printf("header length: %u B\n", tptr->th_off << 2);

    printf("Flags: ");
    if (tptr->th_flags & TH_FIN) printf("FIN ");
    if (tptr->th_flags & TH_SYN) printf("SYN ");
    if (tptr->th_flags & TH_RST) printf("RST ");
    if (tptr->th_flags & TH_PUSH) printf("PUSH ");
    if (tptr->th_flags & TH_ACK) printf("ACK ");
    if (tptr->th_flags & TH_URG) printf("URG ");
    if (tptr->th_flags & 0x40) printf("ECE ");
    if (tptr->th_flags & 0x80) printf("CWR ");
    puts("");
}

u_char *parse_ipv4(const u_char *start, int len) {
    struct iphdr *iptr;
    uint32_t header_len;
    uint32_t saddr, daddr;
    uint8_t a, b, c, d;

    iptr = (struct iphdr *)start;
    if (iptr->version != IPVERSION) {
        fprintf(stderr, "not IPv4 format\n");
        return NULL;
    }
    header_len = iptr->ihl << 2;
    if (header_len != sizeof(struct iphdr)) {
        fprintf(stderr, "header length doesn't match");
    }

    saddr = ntohl(iptr->saddr);
    d = saddr % 256;
    saddr /= 256;
    c = saddr % 256;
    saddr /= 256;
    b = saddr % 256;
    saddr /= 256;
    a = saddr;
    printf("src addr: %u.%u.%u.%u\n", a, b, c, d);
    daddr = ntohl(iptr->daddr);
    d = daddr % 256;
    daddr /= 256;
    c = daddr % 256;
    daddr /= 256;
    b = daddr % 256;
    daddr /= 256;
    a = daddr;
    printf("dest addr: %u.%u.%u.%u\n", a, b, c, d);

    printf("TTL: %u\n", iptr->ttl);
    printf("total length: %u bytes\n", iptr->tot_len);

    switch (iptr->protocol) {
        case IPPROTO_ICMP:
            printf("protocol: ICMP\n");
            break;
        case IPPROTO_ICMPV6:
            printf("protocol: ICMPv6\n");
            break;
        case IPPROTO_UDP:
            printf("protocol: UDP\n");
            parse_udp(start + header_len, len - header_len);
            break;
        case IPPROTO_TCP:
            printf("protocol: TCP\n");
            parse_tcp(start + header_len, len - header_len);
            break;
        default:
            printf("unknown protocol: %u\n", iptr->protocol);
            break;
    }
}

u_char *parse_ipv6(const u_char *start, int len) {
    struct ip6_hdr *iptr;
    iptr = (struct ip6_hdr *)start;
    printf("Payload length: %u\n", iptr->ip6_ctlun.ip6_un1.ip6_un1_plen);
    printf("Hop Limit: %u\n", iptr->ip6_ctlun.ip6_un1.ip6_un1_hlim);
    printf("src addr: ");
    for (int i = 0; i < 8; ++i) {
        printf("%04x%c", ntohs(iptr->ip6_src.__in6_u.__u6_addr16[i]), ":\n"[i == 7]);
    }
    printf("dest addr: ");
    for (int i = 0; i < 8; ++i) {
        printf("%04x%c", ntohs(iptr->ip6_dst.__in6_u.__u6_addr16[i]), ":\n"[i == 7]);
    }
    switch (iptr->ip6_ctlun.ip6_un1.ip6_un1_nxt) {
        case IPPROTO_UDP:
            printf("protocol: UDP\n");
            parse_udp(start + 40, len - 40);
            break;
        case IPPROTO_TCP:
            printf("protocol: TCP\n");
            parse_tcp(start + 40, len - 40);
            break;
        case IPPROTO_ICMP:
            printf("protocol: ICMP\n");
            break;
        case IPPROTO_ICMPV6:
            printf("protocol: ICMPv6\n");
            break;
        default:
            printf("unknown protocol: %u\n", iptr->ip6_ctlun.ip6_un1.ip6_un1_nxt);
            break;
    }
}