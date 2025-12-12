#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

volatile sig_atomic_t wasSigHup = 0;
void sigHupHandler(int r) {
    (void)r;
    wasSigHup = 1;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 5) == -1) {
        perror("listen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8080...\n");

    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGHUP, &sa, NULL);

    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blockedMask, &origMask) == -1) {
        perror("sigprocmask");
        close(sock);
        exit(EXIT_FAILURE);
    }

    int clientSock = -1;

    for (;;) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        int maxFd = sock;

        if (clientSock != -1) {
            FD_SET(clientSock, &fds);
            if (clientSock > maxFd) maxFd = clientSock;
        }

        int result = pselect(maxFd + 1, &fds, NULL, NULL, NULL, &origMask);

        if (result == -1) {
            if (errno == EINTR) {
                // Обработка сигнала (10/11)
                if (wasSigHup) {
                    printf("Received SIGHUP\n");
                    wasSigHup = 0;
                }
                continue;
            } else {
                perror("pselect");
                break;
            }
        }

        if (FD_ISSET(sock, &fds)) {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int newClient = accept(sock, (struct sockaddr*)&clientAddr, &clientLen);
            if (newClient == -1) {
                perror("accept");
            } else {
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);
                printf("New connection from %s:%d\n", ipStr, ntohs(clientAddr.sin_port));

                if (clientSock == -1) {
                    clientSock = newClient;
                } else {
                    printf("Extra connection closed\n");
                    close(newClient);
                }
            }
        }

        if (clientSock != -1 && FD_ISSET(clientSock, &fds)) {
            char buf[1024];
            ssize_t n = read(clientSock, buf, sizeof(buf));
            if (n > 0) {
                printf("Received %zd bytes from client\n", n);
            } else if (n == 0) {
                printf("Client disconnected\n");
                close(clientSock);
                clientSock = -1;
            } else {
                perror("read");
                close(clientSock);
                clientSock = -1;
            }
        }
    }

    if (clientSock != -1) close(clientSock);
    close(sock);
    return 0;
}