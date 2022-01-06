#pragma once
#include "inc.h"
u_char *parse_udp(const u_char *start, int len);
u_char *parse_tcp(const u_char *start, int len);
u_char *parse_ipv4(const u_char *start, int len);
u_char *parse_ipv6(const u_char *start, int len);