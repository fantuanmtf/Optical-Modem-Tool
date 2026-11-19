/*
 * netinfo.c - interfaces, routes, sockets, bridge, ARP and wireless.
 *
 * Everything here is read-only and comes from /proc and /sys where possible.
 * getifaddrs(3) is used when configure found it; /proc/net/dev is the
 * fallback for interface listings.
 */
#include "router-check.h"
#include "zxcompat.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(HAVE_GETIFADDRS) && HAVE_GETIFADDRS
#include <ifaddrs.h>
#endif

#define RC_MAX_IFACES 64

#if defined(HAVE_GETIFADDRS) && HAVE_GETIFADDRS
static void format_flags(unsigned int flags, char *buf, size_t buflen)
{
    static const struct {
        unsigned int bit;
        const char *name;
    } names[] = {
        { IFF_UP, "UP" },
        { IFF_BROADCAST, "BROADCAST" },
        { IFF_RUNNING, "RUNNING" },
        { IFF_LOOPBACK, "LOOPBACK" },
        { IFF_POINTOPOINT, "POINTOPOINT" },
        { IFF_MULTICAST, "MULTICAST" },
        { IFF_PROMISC, "PROMISC" }
    };
    size_t i;

    if (buflen == 0)
        return;
    buf[0] = '\0';
    for (i = 0; i < ZX_ARRAY_LEN(names); i++) {
        if (!(flags & names[i].bit))
            continue;
        if (buf[0] != '\0')
            zx_strlcat(buf, ",", buflen);
        zx_strlcat(buf, names[i].name, buflen);
    }
}

static void report_interface_mac(rc_report *rep, const char *name)
{
    char path[512];
    char buf[64];

    snprintf(path, sizeof(path), "/sys/class/net/%.*s/address",
             IFNAMSIZ - 1, name);
    if (zx_read_file(path, buf, sizeof(buf)) <= 0)
        return;
    zx_trim(buf);
    if (buf[0] != '\0')
        report_line(rep, "    ether %s", buf);
}

static void report_interfaces(rc_report *rep)
{
    struct ifaddrs *list = NULL;
    struct ifaddrs *ifa;
    char names[RC_MAX_IFACES][IFNAMSIZ];
    size_t count = 0;
    size_t i;

    if (getifaddrs(&list) != 0) {
        report_line(rep, "getifaddrs failed; showing /proc/net/dev");
        report_file(rep, "/proc/net/dev");
        return;
    }

    for (ifa = list; ifa != NULL; ifa = ifa->ifa_next) {
        size_t j;
        int seen = 0;

        if (ifa->ifa_name == NULL)
            continue;
        for (j = 0; j < count; j++) {
            if (strcmp(names[j], ifa->ifa_name) == 0) {
                seen = 1;
                break;
            }
        }
        if (seen || count >= RC_MAX_IFACES)
            continue;
        zx_strlcpy(names[count], ifa->ifa_name, IFNAMSIZ);
        count++;
    }

    for (i = 0; i < count; i++) {
        unsigned int flags = 0;
        char flag_text[128];

        for (ifa = list; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_name != NULL &&
                strcmp(ifa->ifa_name, names[i]) == 0) {
                flags = ifa->ifa_flags;
                break;
            }
        }
        format_flags(flags, flag_text, sizeof(flag_text));
        report_line(rep, "%s: flags=%x<%s>", names[i], flags, flag_text);
        report_interface_mac(rep, names[i]);

        for (ifa = list; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_name == NULL ||
                strcmp(ifa->ifa_name, names[i]) != 0 ||
                ifa->ifa_addr == NULL)
                continue;

            if (ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in *sin =
                    (struct sockaddr_in *)ifa->ifa_addr;
                char addr[INET_ADDRSTRLEN] = "";
                char mask[INET_ADDRSTRLEN] = "";

                inet_ntop(AF_INET, &sin->sin_addr, addr, sizeof(addr));
                if (ifa->ifa_netmask != NULL) {
                    struct sockaddr_in *nm =
                        (struct sockaddr_in *)ifa->ifa_netmask;
                    inet_ntop(AF_INET, &nm->sin_addr, mask, sizeof(mask));
                }
                report_line(rep, "    inet %s netmask %s", addr, mask);
            } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                struct sockaddr_in6 *sin6 =
                    (struct sockaddr_in6 *)ifa->ifa_addr;
                char addr[INET6_ADDRSTRLEN] = "";

                inet_ntop(AF_INET6, &sin6->sin6_addr, addr, sizeof(addr));
                report_line(rep, "    inet6 %s", addr);
            }
        }
        report_line(rep, "");
    }
    freeifaddrs(list);
}
#else
static void report_interfaces(rc_report *rep)
{
    report_line(rep, "getifaddrs unavailable; showing /proc/net/dev");
    report_file(rep, "/proc/net/dev");
}
#endif

void rc_network(rc_report *rep)
{
    report_section(rep, "Network interfaces");
    report_interfaces(rep);
    report_line(rep, "-- /proc/net/dev --");
    report_file(rep, "/proc/net/dev");
}

static void value_to_ipv4(unsigned long value, char *out, size_t outlen)
{
    struct in_addr addr;
    unsigned char *bytes = (unsigned char *)&addr.s_addr;

    bytes[0] = (unsigned char)(value & 0xff);
    bytes[1] = (unsigned char)((value >> 8) & 0xff);
    bytes[2] = (unsigned char)((value >> 16) & 0xff);
    bytes[3] = (unsigned char)((value >> 24) & 0xff);
    inet_ntop(AF_INET, &addr, out, outlen);
}

void rc_routes(rc_report *rep)
{
    FILE *fp;
    char line[512];
    int count = 0;

    report_section(rep, "Routes");
    fp = fopen("/proc/net/route", "r");
    if (fp == NULL) {
        report_line(rep, "no /proc/net/route");
        return;
    }
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        report_line(rep, "no IPv4 routes");
        return;
    }

    report_line(rep, "%-12s %-16s %-16s %-6s %-7s %s",
                "Iface", "Destination", "Gateway", "Flags", "Metric",
                "Mask");

    while (fgets(line, sizeof(line), fp) != NULL) {
        char iface[64];
        char dest[INET_ADDRSTRLEN];
        char gateway[INET_ADDRSTRLEN];
        char mask[INET_ADDRSTRLEN];
        unsigned long dest_hex;
        unsigned long gateway_hex;
        unsigned long mask_hex;
        unsigned int flags;
        int metric;

        if (sscanf(line, "%63s %lx %lx %x %*s %*s %d %lx",
                   iface, &dest_hex, &gateway_hex, &flags, &metric,
                   &mask_hex) != 6)
            continue;
        value_to_ipv4(dest_hex, dest, sizeof(dest));
        value_to_ipv4(gateway_hex, gateway, sizeof(gateway));
        value_to_ipv4(mask_hex, mask, sizeof(mask));
        report_line(rep, "%-12s %-16s %-16s %-6x %-7d %s",
                    iface, dest, gateway, flags, metric, mask);
        count++;
    }
    fclose(fp);

    if (count == 0)
        report_line(rep, "no IPv4 routes");
}

/*
 * Parse "0100007F:0050" or a 32-hex-digit IPv6 socket address from
 * /proc/net/{tcp,udp}.  The kernel writes each 32-bit word little-endian.
 */
static int format_socket_address(const char *token, char *out, size_t outlen)
{
    const char *colon = strrchr(token, ':');
    char hex[64];
    size_t hexlen;
    unsigned long port;

    if (colon == NULL)
        return -1;
    hexlen = (size_t)(colon - token);
    if (hexlen == 0 || hexlen >= sizeof(hex))
        return -1;
    memcpy(hex, token, hexlen);
    hex[hexlen] = '\0';
    port = strtoul(colon + 1, NULL, 16);

    if (hexlen == 8) {
        char addr[INET_ADDRSTRLEN];

        value_to_ipv4(strtoul(hex, NULL, 16), addr, sizeof(addr));
        snprintf(out, outlen, "%s:%lu", addr, port);
        return 0;
    }
    if (hexlen == 32) {
        unsigned char bytes[16];
        char addr[INET6_ADDRSTRLEN];
        int word;

        for (word = 0; word < 4; word++) {
            char chunk[9];
            unsigned long value;

            memcpy(chunk, hex + word * 8, 8);
            chunk[8] = '\0';
            value = strtoul(chunk, NULL, 16);
            bytes[word * 4 + 0] = (unsigned char)(value & 0xff);
            bytes[word * 4 + 1] =
                (unsigned char)((value >> 8) & 0xff);
            bytes[word * 4 + 2] =
                (unsigned char)((value >> 16) & 0xff);
            bytes[word * 4 + 3] =
                (unsigned char)((value >> 24) & 0xff);
        }
        if (inet_ntop(AF_INET6, bytes, addr, sizeof(addr)) == NULL)
            return -1;
        snprintf(out, outlen, "[%s]:%lu", addr, port);
        return 0;
    }
    return -1;
}

static int scan_socket_table(rc_report *rep, const char *path,
                             const char *proto, int tcp)
{
    FILE *fp;
    char line[512];
    int count = 0;

    fp = fopen(path, "r");
    if (fp == NULL)
        return 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        char local[128];
        char remote[128];
        char state[16];
        unsigned int uid;
        char address[128];

        if (sscanf(line, " %*d: %127s %127s %15s %*s %*s %*s %u",
                   local, remote, state, &uid) != 4)
            continue;
        if (tcp && strcmp(state, "0A") != 0)
            continue;
        if (!tcp && strcmp(state, "07") != 0)
            continue;
        if (format_socket_address(local, address, sizeof(address)) != 0)
            continue;
        report_line(rep, "%-5s %-44s %s", proto, address,
                    tcp ? "LISTEN" : "UNCONN");
        count++;
    }
    fclose(fp);
    return count;
}

void rc_listening(rc_report *rep)
{
    int count = 0;

    report_section(rep, "Listening sockets");
    count += scan_socket_table(rep, "/proc/net/tcp", "tcp", 1);
    count += scan_socket_table(rep, "/proc/net/tcp6", "tcp6", 1);
    count += scan_socket_table(rep, "/proc/net/udp", "udp", 0);
    count += scan_socket_table(rep, "/proc/net/udp6", "udp6", 0);

    if (count == 0)
        report_line(rep, "no listening TCP/UDP sockets found");
}

void rc_arp(rc_report *rep)
{
    FILE *fp;
    char line[512];
    int count = 0;

    report_section(rep, "ARP");
    fp = fopen("/proc/net/arp", "r");
    if (fp == NULL) {
        report_line(rep, "no /proc/net/arp");
        return;
    }
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        report_line(rep, "ARP table is empty");
        return;
    }

    report_line(rep, "%-16s %-19s %-6s %s", "IP address", "HW address",
                "Flags", "Device");
    while (fgets(line, sizeof(line), fp) != NULL) {
        char ip[64];
        char hw[64];
        char device[64];
        unsigned int type;
        unsigned int flags;

        if (sscanf(line, "%63s %x %x %63s %*s %63s",
                   ip, &type, &flags, hw, device) != 5)
            continue;
        report_line(rep, "%-16s %-19s %-6x %s", ip, hw, flags, device);
        count++;
    }
    fclose(fp);

    if (count == 0)
        report_line(rep, "ARP table is empty");
}

static void bridge_id_for(const char *name, char *out, size_t outlen)
{
    char path[512];
    char buf[64];

    snprintf(path, sizeof(path), "/sys/class/net/%s/bridge/bridge_id", name);
    if (zx_read_file(path, buf, sizeof(buf)) < 0) {
        zx_strlcpy(out, "-", outlen);
        return;
    }
    zx_strlcpy(out, zx_trim(buf), outlen);
}

static void bridge_members_for(const char *name, char *out, size_t outlen)
{
    char path[512];
    DIR *dir;
    struct dirent *entry;

    out[0] = '\0';
    snprintf(path, sizeof(path), "/sys/class/net/%s/brif", name);
    dir = opendir(path);
    if (dir == NULL)
        return;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        if (out[0] != '\0')
            zx_strlcat(out, " ", outlen);
        zx_strlcat(out, entry->d_name, outlen);
    }
    closedir(dir);
}

void rc_bridge(rc_report *rep)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    report_section(rep, "Bridge");
    dir = opendir("/sys/class/net");
    if (dir == NULL) {
        report_line(rep, "no /sys/class/net");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char path[512];
        char id[64];
        char stp_path[512];
        char stp[32] = "-";
        char members[1024];
        char buf[64];
        struct stat st;

        if (entry->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "/sys/class/net/%s/bridge",
                 entry->d_name);
        if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode))
            continue;

        if (count == 0) {
            report_line(rep, "%-12s %-20s %-6s %s", "Bridge", "Bridge ID",
                        "STP", "Interfaces");
        }
        bridge_id_for(entry->d_name, id, sizeof(id));
        snprintf(stp_path, sizeof(stp_path),
                 "/sys/class/net/%s/bridge/stp_state", entry->d_name);
        if (zx_read_file(stp_path, buf, sizeof(buf)) >= 0)
            zx_strlcpy(stp, zx_trim(buf), sizeof(stp));
        bridge_members_for(entry->d_name, members, sizeof(members));
        report_line(rep, "%-12s %-20s %-6s %s", entry->d_name, id, stp,
                    members);
        count++;
    }
    closedir(dir);

    if (count == 0)
        report_line(rep, "no bridge interfaces found");
}

void rc_wireless(rc_report *rep)
{
    char buf[4096];
    char *line;
    int data_lines = 0;

    report_section(rep, "Wireless");
    if (zx_read_file("/proc/net/wireless", buf, sizeof(buf)) >= 0) {
        line = buf;
        while (*line != '\0') {
            char *next = strchr(line, '\n');
            if (next != NULL)
                *next = '\0';
            if (strchr(line, ':') != NULL)
                data_lines++;
            if (next == NULL)
                break;
            line = next + 1;
        }
        if (data_lines > 0) {
            report_file(rep, "/proc/net/wireless");
            return;
        }
    }

    if (report_popen(rep, "iwconfig 2>/dev/null") == 0)
        report_line(rep, "no wireless information available");
}

