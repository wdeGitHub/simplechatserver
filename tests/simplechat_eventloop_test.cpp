#include "simplechat.h"

#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
constexpr int kTestPort = 19090;

void runServer() {
    SimpleChat server;
    server.init("127.0.0.1", "root", "pass", "db", kTestPort, 1, 1);
    server.eventListen();
    server.eventLoop();
}
}

int main() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        try {
            runServer();
        } catch (const std::exception &ex) {
            std::cerr << "[server] exception: " << ex.what() << std::endl;
        }
        _exit(0);
    }

    sleep(1);

    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1) {
        perror("socket");
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kTestPort);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
        perror("inet_pton");
        close(cfd);
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
        return 1;
    }

    if (connect(cfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("connect");
        close(cfd);
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
        return 1;
    }

    const char msg[] = "ping";
    if (send(cfd, msg, sizeof(msg), 0) == -1) {
        perror("send");
    }

    char buf[64] = {0};
    ssize_t len = recv(cfd, buf, sizeof(buf), 0);
    if (len > 0) {
        std::cout << "[client] echo: " << std::string(buf, len) << std::endl;
    } else if (len == 0) {
        std::cout << "[client] server closed connection" << std::endl;
    } else {
        perror("recv");
    }

    close(cfd);
    kill(pid, SIGTERM);
    waitpid(pid, nullptr, 0);
    return 0;
}

