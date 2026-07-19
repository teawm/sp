#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>

#define BUFFER_SIZE 1024

void color(int color)
{
    printf("\033[0;3%dm", color);
}

int main(int argc, char *argv[])
{
    std::cout << "\033[2J\033[H" << std::endl;
    if (argc < 2)
    {
        color(1);
        std::cerr << "./client <server-ip> <server-port> <value 0-9 def=random>* <time def=3>* <repeats def=5>*" << std::endl;
        return -1;
    }
    srand(time(NULL));
    const char *server_ip = argv[1];
    int port = std::atoi(argv[2]);
    int value = rand() % 10;
    int iterations = 5;
    int time = 3;

    std::cout << argc;

    if (argc > 3)
        value = std::atoi(argv[3]);
    if (argc > 4)
        time = std::atoi(argv[4]);
    if (argc > 5)
        iterations = std::atoi(argv[5]);
    value = value % 10;
    time = time % 10;
    iterations = iterations % 16;
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        color(1);
        perror("[socket creation failed.]");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        color(1);
        std::cerr << "[Invalid address / Address not supported.]" << std::endl;
        close(sockfd);
        return -1;
    }
    color(0);
    std::cout << "\033[2;22H[\t\t\t ]";
    std::cout << "\033[3;22H[\t\t\t ]";
    std::cout << "\033[4;22H[\t\t\t ]";
    std::cout << "\033[5;22H[\t\t\t ]";

    color(2);

    std::cout << "\033[2;34H╬";
    std::cout << "\033[3;30H╬\t    ╗";
    std::cout << "\033[4;34H╔══╝";
    std::cout << "\033[5;29H╚════╝";

    color(1);
    std::cout << "\033[2;1H[UDP CLIENT STARTED.]\n";
    color(1);
    std::cout << "\033[3;1H[SERVER CONNECT INFO]\n";
    color(1);
    std::cout << "[IP:\t\t    ]\n[PORT:\t\t    ]" << std::endl;
    color(4);
    std::cout << "\033[4;9H" << server_ip;
    color(4);
    std::cout << "\033[5;11H" << port << std::endl;
    color(1);
    std::cout << "[CLIENT CONNECT INFO]";
    color(1);
    std::cout << "[input value:   ";
    color(4);
    std::cout << value;
    color(1);
    std::cout << "   ]\n[input time:   /";
    color(4);
    std::cout << time;
    color(1);
    std::cout << "   ][iterations:   /";
    color(4);
    std::cout << std::setw(2) << iterations;
    color(1);
    std::cout << "  ]";

    std::cout << "\033[8;1H";
    color(1);
    std::cout << "[";
    color(3);
    std::cout << "  Sent for server: ";
    color(1);
    std::cout << "][";
    color(3);
    std::cout << "  Got from server: ";
    color(1);
    std::cout << "]\n";

    std::cout << "\033[9;1H";

    for (int i = 0; i < iterations; i++)
    {
        snprintf(buffer, BUFFER_SIZE, "%d", value);

        sendto(sockfd, buffer, strlen(buffer), 0,
               (const struct sockaddr *)&server_addr, sizeof(server_addr));
        int x = i + 9;

        if (i < iterations)
        {
            printf("\033[7;15H0");
            for (int j = 0; j < time; j++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(700));
                printf("\033[7;15H%d", j + 1);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
        }

        printf("\033[7;36H");
        color(4);
        std::cout << i + 1;

        printf("\033[%d;1H", x);
        color(1);
        std::cout << "[";
        color(2);
        std::cout << buffer;
        color(3);
        std::cout << " > ";
        color(2);
        std::cout << server_ip << ":" << port;
        color(1);
        std::cout << "]";

        memset(buffer, 0, BUFFER_SIZE);
        socklen_t server_len = sizeof(server_addr);
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                             (struct sockaddr *)&server_addr, &server_len);

        if (n > 0)
        {
            color(1);
            std::cout << "[";
            color(2);
            std::cout << server_ip << ":" << port;
            color(3);
            std::cout << " > ";
            color(2);
            std::cout << buffer;
            color(1);
            std::cout << "]" << std::endl;
        }
    }

    close(sockfd);
    color(1);
    std::cout << "[  CLIENT FINISHED. ][  CLIENT FINISHED. ]" << std::endl;
    return 0;
}