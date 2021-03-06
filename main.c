#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "nat_traversal.h"

#define DEFAULT_SERVER_PORT 9988
#define MSG_BUF_SIZE 512

// use some public stun servers to detect port allocation rule
static char *stun_servers[] = {
    "stun.ideasip.com",
    "stun.ekiga.net",
    "203.183.172.196"
};

int main(int argc, char** argv)
{
    char* stun_server = stun_servers[0];
    char* local_host = "0.0.0.0";
    uint16_t stun_port = DEFAULT_STUN_SERVER_PORT;
    uint16_t local_port = DEFAULT_LOCAL_PORT;
    char* punch_server = NULL;
    uint32_t peer_id = 0;

    static char usage[] = "usage: [-h] [-H STUN_HOST] [-P STUN_PORT] [-s punch server] [-d id] [-i SOURCE_IP] [-p SOURCE_PORT]\n";
    int opt;
    while ((opt = getopt (argc, argv, "H:h:P:p:s:d:i")) != -1)
    {
        switch (opt)
        {
            case 'h':
                printf("%s", usage);
                break;
            case 'H':
                stun_server = optarg;
                break;
            case 'P':
                stun_port = atoi(optarg);
                break;
            case 's':
                punch_server = optarg;
                break;
            case 'p':
                local_port = atoi(optarg);
                break;
            case 'd':
                peer_id = atoi(optarg);
                break;
            case 'i':
                local_host = optarg;
                break;
            case '?':
            default:
                printf("invalid option: %c\n", opt);
                printf("%s", usage);

                return -1;
        }
    }

    char ext_ip[16] = {0};
    uint16_t ext_port = 0;

    nat_type type = detect_nat_type(stun_server, stun_port, local_host, local_port, ext_ip, &ext_port);

    // TODO log
    printf("NAT type: %s\n", get_nat_desc(type));
    if (ext_port) {
        printf("external address: %s:%d\n", ext_ip, ext_port);
    } else {
        return -1;
    }

    if (!punch_server) {
        printf("please specify punch server\n");
        return -1;
    }
    struct peer_info self;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(punch_server);
    server_addr.sin_port = htons(DEFAULT_SERVER_PORT);

    client c;

    // test
    c.type = SymmetricNAT;

    if (init(self, server_addr, &c)) {
        printf("init failed\n");

        return -1;
    }

    if (peer_id) {
        if (connect_to_peer(&c, peer_id) < 0) {
            printf("failed to connect\n");

            return -1;
        }
    }

    return 0;
}
