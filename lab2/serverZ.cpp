#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <cstdlib>

#define BUFFER_SIZE 1024

// Для завершения зомби-процессов
/*
void sigchld_handler(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
    }
}
*/

int main()
{
    int server_sockfd, client_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Установка обработчика для завершения зомби-процессов
    // signal(SIGCHLD, sigchld_handler);

    // Создание TCP сокета
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation failed");
        return -1;
    }

    // Опция для быстрого переиспользования порта
    int opt = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Настройка адреса сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(0);

    // Привязка сокета
    if (bind(server_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(server_sockfd);
        return -1;
    }

    // Получение фактически назначенного порта
    struct sockaddr_in assigned_addr;
    socklen_t addr_len = sizeof(assigned_addr);
    if (getsockname(server_sockfd, (struct sockaddr *)&assigned_addr, &addr_len) < 0)
    {
        perror("getsockname failed");
        close(server_sockfd);
        return -1;
    }

    int actual_port = ntohs(assigned_addr.sin_port);

    // Прослушивание подключений (очередь до 10 клиентов)
    if (listen(server_sockfd, 10) < 0)
    {
        perror("listen failed");
        close(server_sockfd);
        return -1;
    }

    std::cout << "[TCP FORK Сервер запущен.]" << std::endl;
    std::cout << "[Прослушиваемый порт]: " << actual_port << std::endl;
    std::cout << "[Ожидание клиентов...]" << std::endl
              << std::endl;
    std::cout << "\n\033[0;31m <ЗОМБИ-ПРОЦЕССЫ> \033[0;37m\n" << std::endl;

    // Основной цикл принятия соединений
    while (true)
    {
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &client_len);

        if (client_sockfd < 0)
        {
            perror("accept failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);

        printf("\033[0;3%dm", client_port % 6 + 1);

        std::cout << "[НОВЫЙ КЛИЕНТ]: [" << client_ip << ":" << client_port << "]" << std::endl;

        // Создание дочернего процесса для обработки клиента
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork failed");
            close(client_sockfd);
            continue;
        }

        if (pid == 0)
        {
            // ДОЧЕРНИЙ ПРОЦЕСС (обработка одного клиента)
            close(server_sockfd); // Закрытие слушающего сокета в дочернем процессе

            while (true)
            {
                printf("\033[0;3%dm", client_port % 6 + 1);
                memset(buffer, 0, BUFFER_SIZE);

                ssize_t n = recv(client_sockfd, buffer, BUFFER_SIZE, 0);

                if (n <= 0)
                {
                    if (n == 0)
                    {
                        std::cout << "[КЛИЕНТ " << client_ip << ":" << client_port
                                  << "]---> [Отключено.]" << std::endl
                                  << std::endl;
                    }
                    else
                    {
                        perror("recv error");
                    }
                    break;
                }

                std::cout << "[КЛИЕНТ " << client_ip << ":" << client_port
                          << "]-+-> [ Получено ]: " << buffer;

                int num = std::atoi(buffer);
                int reply = client_port % 6 + 1;

                snprintf(buffer, BUFFER_SIZE, "%d", reply);

                // Отправка ответа клиенту
                send(client_sockfd, buffer, strlen(buffer), 0);

                std::cout << "\n\t\t\t +-> [Отправлено]: " << reply << std::endl;
            }

            close(client_sockfd);
            exit(0); // Завершение дочернего процесса
        }
        else
        {
            // РОДИТЕЛЬСКИЙ ПРОЦЕСС
            close(client_sockfd); // Закрытие клиентского сокета в родительском процессе
        }
    }

    close(server_sockfd);
    return 0;
}