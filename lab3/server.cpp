#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fstream>
#include <ctime>
#include <cerrno>

#define BUFFER_SIZE 1024

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
const char* LOG_FILE = "server_log.txt";

struct ClientInfo {
    int sockfd;
    struct sockaddr_in addr;
};

std::string get_timestamp() {
    time_t now = time(0);
    struct tm *ltm = localtime(&now);
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ltm);
    return std::string(buffer);
}

void* handle_client(void* arg) {
    ClientInfo* client = (ClientInfo*)arg;
    int client_sockfd = client->sockfd;
    struct sockaddr_in client_addr = client->addr;
    delete client;
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);
    
    std::cout << "[НОВЫЙ КЛИЕНТ] " << client_ip << ":" << client_port << std::endl;
    
    char buffer[BUFFER_SIZE];
    
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        
        ssize_t n = recv(client_sockfd, buffer, BUFFER_SIZE - 1, 0);
        
        if (n <= 0) {
            if (n == 0) {
                std::cout << "[КЛИЕНТ " << client_ip << ":" << client_port 
                          << "] Отключён" << std::endl;
            } else {
                perror("recv error");
            }
            break;
        }
        
        buffer[strcspn(buffer, "\n\r")] = '\0';
        
        std::cout << "[КЛИЕНТ " << client_ip << ":" << client_port 
                  << "] Получено: " << buffer << std::endl;
        
        pthread_mutex_lock(&file_mutex);
        
        std::ofstream log_file(LOG_FILE, std::ios::app);
        if (log_file.is_open()) {
            log_file << "[" << get_timestamp() << "] "
                     << "Клиент " << client_ip << ":" << client_port
                     << " -> Данные: " << buffer << std::endl;
            log_file.close();
            std::cout << "  [ЗАПИСАНО В ФАЙЛ: " << LOG_FILE << "]" << std::endl;
        } else {
            std::cerr << "  [ОШИБКА: не удалось открыть файл " << LOG_FILE << "]" << std::endl;
        }
        
        pthread_mutex_unlock(&file_mutex); 

        const char* response = "OK";
        send(client_sockfd, response, strlen(response), 0);
    }
    
    close(client_sockfd);
    pthread_exit(NULL);
}

int main() {
    int server_sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }
    
    int opt = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(0);
    
    if (bind(server_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_sockfd);
        return -1;
    }
    
    struct sockaddr_in assigned_addr;
    socklen_t addr_len = sizeof(assigned_addr);
    if (getsockname(server_sockfd, (struct sockaddr*)&assigned_addr, &addr_len) < 0) {
        perror("getsockname failed");
        close(server_sockfd);
        return -1;
    }
    
    int actual_port = ntohs(assigned_addr.sin_port);
    
    if (listen(server_sockfd, 10) < 0) {
        perror("listen failed");
        close(server_sockfd);
        return -1;
    }
    
    std::cout << "=== МНОГОПОТОЧНЫЙ TCP СЕРВЕР (pthread) ===" << std::endl;
    std::cout << "[Порт]: " << actual_port << std::endl;
    std::cout << "[Лог-файл]: " << LOG_FILE << std::endl;
    std::cout << "[Ожидание клиентов...]" << std::endl << std::endl;
    
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_sockfd < 0) {
            perror("accept failed");
            continue;
        }
        
        ClientInfo* client_info = new ClientInfo();
        client_info->sockfd = client_sockfd;
        client_info->addr = client_addr;
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client_info) != 0) {
            perror("pthread_create failed");
            close(client_sockfd);
            delete client_info;
            continue;
        }
        
        pthread_detach(thread_id);
    }
    
    close(server_sockfd);
    return 0;
}