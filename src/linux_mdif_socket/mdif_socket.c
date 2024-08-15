/*******************************************************************************
 *                                                                             *
 *                                                 ,,                          *
 *                                                       ,,,,,                 *
 *                                                           ,,,,,             *
 *           ,,,,,,,,,,,,,,,,,,,,,,,,,,,,                        ,,,,          *
 *          ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,            ,,,,          ,,,,        *
 *          ,,,,,       ,,,,,      ,,,,,,                ,,,,        ,,,       *
 *          ,,,,,       ,,,,,      ,,,,,,                   ,,,        ,,,     *
 *          ,,,,,       ,,,,,      ,,,,,,       ,,,           ,,,        ,     *
 *          ,,,,,       ,,,,,      ,,,,,,           ,,,         ,,        ,    *
 *          ,,,,,       ,,,,,      ,,,,,,              ,,        ,,            *
 *          ,,,,,       ,,,,,      ,,,,,,                ,        ,            *
 *          ,,,,,       ,,,,,      ,,,,,,                 ,                    *
 *          ,,,,,       ,,,,,      ,,,,,,                                      *
 *          ,,,,,       ,,,,,      ,,,,,,                                      *
 *                                       ,,,,,,,,,,,,,,,,,,,,,,,,,,            *
 *                                       ,,,,,,,,,,,,,,,,,,,,,,,,,,,,          *
 *                                       ,,,,,                  ,,,,,,         *
 *                     ,                 ,,,,,                  ,,,,,,         *
 *             ,        ,,               ,,,,,                  ,,,,,,         *
 *    ,        ,,        ,,,             ,,,,,                  ,,,,,,         *
 *     ,        ,,,         ,,,          ,,,,,                  ,,,,,,         *
 *     ,,,       ,,,                     ,,,,,                  ,,,,,,         *
 *      ,,,        ,,,,                  ,,,,,                  ,,,,,,         *
 *        ,,,         ,,,,               ,,,,,                  ,,,,,,         *
 *         ,,,,,            ,,,,         ,,,,,,,,,,,,,,,,,,,,,,,,,,,,          *
 *            ,,,,                       ,,,,,,,,,,,,,,,,,,,,,,,,,,            *
 *               ,,,,,                                                         *
 *                    ,,,,,                                                    *
 *                                                                             *
 * Program/file : main.c                                                       *
 *                                                                             *
 * Description  : Provide the user an interface to prompt protobuf messages to *
 *              : the device running mdif over serial connection using hdlc.   *
 *                                                                             *
 * Copyright 2023 MyDefence A/S.                                               *
 *                                                                             *
 * Licensed under the Apache License, Version 2.0 (the "License");             *
 * you may not use this file except in compliance with the License.            *
 * You may obtain a copy of the License at                                     *
 *                                                                             *
 * http://www.apache.org/licenses/LICENSE-2.0                                  *
 *                                                                             *
 * Unless required by applicable law or agreed to in writing, software         *
 * distributed under the License is distributed on an "AS IS" BASIS,           *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.    *
 * See the License for the specific language governing permissions and         *
 * limitations under the License.                                              *
 *                                                                             *
 *                                                                             *
 *                                                                             *
 *******************************************************************************/
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include "codec.h"

static pthread_t rx_thread;

int mdif_socket = -1;

static int mdif_socket_connect(const char *host) {
    const char *port = "21020";
    char *colon = strchr(host, ':');
    if (colon != NULL) {
        *colon = '\0';
        port = colon + 1;
    }

    //  struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s = getaddrinfo(host, port, NULL, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }

    while (1) {
        printf("Connecting to %s:%s\n", host, port);

        int tmp_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (tmp_sock == -1) {
            perror("socket");
            exit(1);
        }

        while (1) {
            for (rp = result; rp != NULL; rp = rp->ai_next) {
                if (connect(tmp_sock, rp->ai_addr, rp->ai_addrlen) != -1) {
                    printf("Connected\n");
                    freeaddrinfo(result);
                    return tmp_sock;
                }
                printf("connect failed: %s\n", strerror(errno));
            }
            sleep(2);
        }
    }
}

// Receive exactly n bytes from sock. Returns -1 if that fails, otherwise 0
static int recvn(int sock, void *buf, size_t n)
{
    int len = recv(sock, buf, n, MSG_WAITALL);
    if (len == -1) {
        perror("recv");
        return -1;
    }
    if (len == 0) {
        return -1;
    }
    if (len != n) {
        printf("incomplete length (%d vs %ld)\n", len, n);
        return -1;
    }
    return 0;
}

static void *rx_thread_func(void *ptr)
{
    while (1) {
        uint32_t pblen;
        if (recvn(mdif_socket, &pblen, sizeof(pblen)) == -1) {
            exit(1);
        }
        pblen = le32toh(pblen);

        uint8_t *pb = malloc(pblen);
        if (recvn(mdif_socket, pb, pblen) == -1) {
            exit(1);
        }
        printf("Received %d bytes\n", pblen);
        decode_mdif_msg(pb, pblen);

        free(pb);
    }
}

void mdif_socket_init(const char *host)
{
    mdif_socket = mdif_socket_connect(host);
    if (pthread_create(&rx_thread, NULL, rx_thread_func, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
}


void mdif_socket_send(const uint8_t *buf, uint32_t size)
{
    if (mdif_socket == -1) {
        printf("Not connected\n");
        return;
    }
    uint32_t pblen = htole32(size);
    if (send(mdif_socket, &pblen, sizeof(pblen), 0) == -1) {
        perror("send");
        exit(1);
    }
    if (send(mdif_socket, buf, size, 0) == -1) {
        perror("send");
        exit(1);
    }
}
