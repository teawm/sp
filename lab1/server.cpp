#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctime>

#define BUFFER_SIZE 1024

void color(int color){
    printf("\033[0;3%dm", color);
}

int main() {
    std::cout << "\033[2J\033[H" << std::endl;
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        color(1);
        perror("[socket creation failed.]");
        return -1;
    }
    
    
    memset(&server_addr, 0, sizeof(server_addr));
    const char* ip = "127.0.0.1";
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(0);
    
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        color(1);
        perror("[bind failed.]");
        close(sockfd);
        return -1;
    }
    
    struct sockaddr_in assigned_addr;
    socklen_t addr_len = sizeof(assigned_addr);
    if (getsockname(sockfd, (struct sockaddr*)&assigned_addr, &addr_len) < 0) {
        color(1);
        perror("[getsockname failed].");
        close(sockfd);
        return -1;
    }
    color(0);
    std::cout << "\033[2;22H[\t\t\t ]";
    std::cout << "\033[3;22H[\t\t\t ]";
    std::cout << "\033[4;22H[\t\t\t ]";
    
    color(2);
    std::cout << "\033[2;34H║══╗";
    std::cout << "\033[3;28H\\\\//  ║══╝";
    std::cout << "\033[4;28H//\\\\  ║";
    
    color(5); std::cout << "\033[2;1H[UDP SERVER STARTED.]\n";
    color(5); std::cout << "[IP:\t\t    ]\n[PORT:\t\t    ]" << std::endl;
    color(5); std::cout << "[";
    color(3); std::cout << " Got from clients: ";
    color(5); std::cout << "][";
    color(3); std::cout << " Sent for clients: ";
    color(5); std::cout << "]";
    color(2); std::cout << "\033[3;9H" << ip;
    color(2); std::cout << "\033[4;11H" << ntohs(assigned_addr.sin_port) << std::endl;
    
    std::cout << "\033[6;1H";
    
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                            (struct sockaddr *)&client_addr, &client_len);
        
        if (n < 0) {
            color(1);
            perror("[recvfrom failed.]");
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);
        color(5); std::cout << "["; 
        color(2); std::cout << client_ip << ":" << client_port;
        color(3); std::cout << " > ";
        color(2); std::cout << buffer;
        color(5); std::cout << "]";
        
        for (int i = 0; i < n && buffer[i] != '\0'; i++) {
            if (isdigit(buffer[i])) {
                int num = buffer[i] - '0';
                buffer[i] = '0' + (9 - num);
            }
        }
        
        sendto(sockfd, buffer, strlen(buffer), 0,
               (const struct sockaddr *)&client_addr, client_len);
        
        color(5); std::cout << "["; 
        color(2); std::cout << buffer;
        color(3); std::cout << " > ";
        color(2); std::cout << client_ip << ":" << client_port;
        color(5); std::cout << "]" << std::endl;
    }
    
    close(sockfd);
    return 0;
}