#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <chrono>
#include <thread>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Использование: " << argv[0] << " <server_ip> <port> <number>" << std::endl;
        std::cerr << "Пример: " << argv[0] << " 127.0.0.1 54321 2" << std::endl;
        return -1;
    }
    
    const char* server_ip = argv[1];
    int port = std::atoi(argv[2]);
    int number = std::atoi(argv[3]);
    int iterations = 8;
    
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        std::cerr << "неверный адрес / адрес не поддерживается" << std::endl;
        close(sockfd);
        return -1;
    }
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        close(sockfd);
        return -1;
    }
    
    std::cout << "=== TCP КЛИЕНТ ===" << std::endl;
    std::cout << "[Сервер]: " << server_ip << ":" << port << std::endl;
    std::cout << "[Отправка числа " << number << " каждые " << number 
              << " сек (" << iterations << " раз)]" << std::endl << std::endl;
    
    for (int i = 0; i < iterations; i++) {
        snprintf(buffer, BUFFER_SIZE, "%d", number);
        
        send(sockfd, buffer, strlen(buffer), 0);
        std::cout << "[" << i+1 << "/" << iterations << "] Отправлено: " << buffer << std::endl;
        
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t n = recv(sockfd, buffer, BUFFER_SIZE, 0);
        
        if (n > 0) {
            std::cout << "           Получено: " << buffer << std::endl << std::endl;
        } else {
            std::cerr << "Сервер закрыл соединение" << std::endl;
            break;
        }
        
        if (i < iterations - 1) {
            std::this_thread::sleep_for(std::chrono::seconds(number));
        }
    }
    
    close(sockfd);
    std::cout << "=== КЛИЕНТ ЗАВЕРШЕН ===" << std::endl;
    return 0;
}