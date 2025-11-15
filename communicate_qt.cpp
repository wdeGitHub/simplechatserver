#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <wchar.h>
#include <locale.h>

#define PORT 9990
#define BUFFER_SIZE 1024

// 处理客户端连接的线程函数
void *handle_client(void *client_fd_ptr) {
    int client_fd = *(int *)client_fd_ptr;
    free(client_fd_ptr); // 释放动态分配的内存

    // 设置 locale 为 UTF-8，确保宽字符处理正常
    setlocale(LC_ALL, "en_US.UTF-8");
    wprintf(L"客户端已连接（fd: %d）\n", client_fd);

    char buffer[BUFFER_SIZE];
    ssize_t recv_len;

    while (1) {
        // 接收客户端数据（按换行符分割，避免粘包）
        memset(buffer, 0, BUFFER_SIZE);
        recv_len = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (recv_len <= 0) {
            if (recv_len < 0) perror("read error");
            break; // 客户端断开连接
        }

        // 移除末尾的换行符（Qt 客户端添加的）
        buffer[strcspn(buffer, "\n")] = '\0';
        printf("收到客户端（fd: %d）：%s（UTF-8 字节流）\n", client_fd, buffer);

        // 验证并解析为宽字符串（UTF-8 → wchar_t）
        wchar_t wstr[BUFFER_SIZE];
        if (mbstowcs(wstr, buffer, BUFFER_SIZE) == (size_t)-1) {
            printf("警告：非法的 UTF-8 字节流\n");
            const char *err_msg = "收到非法 UTF-8 数据\n";
            send(client_fd, err_msg, strlen(err_msg), 0);
            continue;
        }
        wprintf(L"解析为宽字符串：%ls\n", wstr);

        // 回复客户端（UTF-8 格式）
        char reply[BUFFER_SIZE];
        snprintf(reply, BUFFER_SIZE, "服务端已收到：%s\n", buffer);
        send(client_fd, reply, strlen(reply), 0);
    }

    printf("客户端(fd: %d)断开连接\n", client_fd);
    close(client_fd);
    return NULL;
}

int main() {
    int server_fd, new_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 创建 socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置端口复用
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // 绑定端口和 IP
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 监听连接（最大等待队列 5）
    if (listen(server_fd, 5) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("服务端启动，监听端口 %d...\n", PORT);

    // 循环接受客户端连接（多线程处理）
    while (1) {
        if ((new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
            perror("accept failed");
            continue;
        }

        // 打印客户端 IP 和端口
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("新连接：%s:%d（fd: %d）\n", client_ip, ntohs(client_addr.sin_port), new_fd);

        // 创建线程处理客户端
        pthread_t thread_id;
        int *client_fd_ptr = (int*)malloc(sizeof(int));
        *client_fd_ptr = new_fd;
        if (pthread_create(&thread_id, NULL, handle_client, client_fd_ptr) != 0) {
            perror("pthread_create failed");
            close(new_fd);
            free(client_fd_ptr);
        }
        pthread_detach(thread_id); // 线程结束后自动释放资源
    }

    close(server_fd);
    return 0;
}
