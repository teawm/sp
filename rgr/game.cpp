#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <cctype>
#include <termios.h>
#include <csignal>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>

using namespace std;

#define PORT 8080

struct NetPacket
{
    int type;
    int col;
};

termios old_term;
bool logs = false;
void restorer(int)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    cout << "\033[?25h\033[?8c";
    exit(0);
}

bool wideness = false;
int Matrix[6][7];
int map_x = 0, map_y = 0;

int sockfd = -1;
pthread_t net_tid;
pthread_mutex_t net_mtx;
pthread_cond_t net_cnd;
bool remote_ready = false;
int remote_col = -1;
bool my_turn = false;

pthread_mutex_t console_mtx = PTHREAD_MUTEX_INITIALIZER;
int last_remote_cursor_col = -1;

void init(int M[6][7])
{
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 7; j++)
            M[i][j] = 0;
}

bool checkGame()
{
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            int sum = 0;
            for (int d = 0; d < 4; d++)
            {
                sum += Matrix[i][j + d];
            }
            if (sum >= 4 || sum <= -4)
            {
                return false;
            }
        }
    }

    for (int j = 0; j < 7; j++)
    {
        for (int i = 0; i < 3; i++)
        {
            int sum = 0;
            for (int d = 0; d < 4; d++)
            {
                sum += Matrix[i + d][j];
            }
            if (sum >= 4 || sum <= -4)
            {
                return false;
            }
        }
    }

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            int sum = 0;
            for (int d = 0; d < 4; d++)
            {
                sum += Matrix[i + d][j + d];
            }
            if (sum >= 4 || sum <= -4)
            {
                return false;
            }
        }
    }

    for (int i = 3; i < 6; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            int sum = 0;
            for (int d = 0; d < 4; d++)
            {
                sum += Matrix[i - d][j + d];
            }
            if (sum >= 4 || sum <= -4)
            {
                return false;
            }
        }
    }

    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 7; j++)
        {
            if (Matrix[i][j] == 0)
                return true;
        }
    }
    return false;
}

void printLogMap(int M[6][7])
{
    if (logs)
    {
        int wide = 70;
        if (wideness)
            wide = 74;
        for (int i = 0; i < 6; i++)
        {
            printf("\033[%d;%dH", i - 14 + (wide / 3), wide);
            for (int j = 0; j < 7; j++)
                cout << setw(2) << M[i][j] << " ";
            cout << endl;
        }
    }
}

void interfaceBlock(int x, int y, char *str)
{
    char buff[12];
    printf("\033[%d;%dH", x, y);
    printf("\033(0%s\033(B", str);
}

void interface(int y1, int x1, int dy, int dx)
{
    interfaceBlock(x1, y1, "l");
    interfaceBlock(x1 + dx - 1, y1, "m");
    interfaceBlock(x1, y1 + dy - 1, "k");
    interfaceBlock(x1 + dx - 1, y1 + dy - 1, "j");

    for (int i = 1; i < dx - 1; i++)
    {
        interfaceBlock(x1 + i, y1, "x");
        interfaceBlock(x1 + i, y1 + dy - 1, "x");
    }

    for (int i = 1; i < dy - 1; i++)
    {
        interfaceBlock(x1, y1 + i, "q");
        interfaceBlock(x1 + dx - 1, y1 + i, "q");
    }
}

void printMap()
{
    cout << endl;
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 7; j++)
            if (wideness)
            {
                printf("\033[%d;%dH   ▄▄", i * 5 + 2 + map_y, j * 10 + 1 + map_x);
                printf("\033[%d;%dH   ▀▀", i * 5 + 3 + map_y, j * 10 + 1 + map_x);
            }
            else
                printf("\033[%d;%dH   ■", i * 4 + 1 + map_y, j * 9 + 1 + map_x);
    }
}

void printCell(int y, int x, int color, bool choose)
{
    if (color == -1)
        color = 4;
    int style = 0;
    if (color == 3)
        style = 2;
    if (wideness)
    {
        x = x * 10 + map_x;
        y = y * 5 + map_y + 1;
        if (choose)
        {
            y--;
        }
        printf("\033[%d;3%dm", style, color);
        printf("\033[%d;%dH▄█▀▀▀▀█▄\n", y, x + 1);
        printf("\033[%d;%dH█  ▄▄  █\n", y + 1, x + 1);
        printf("\033[%d;%dH█  ▀▀  █\n", y + 2, x + 1);
        printf("\033[%d;%dH▀█▄▄▄▄█▀\n", y + 3, x + 1);
    }
    else
    {
        x = x * 9 + map_x;
        y = y * 4 + map_y;
        if (choose)
        {
            y--;
        }
        printf("\033[%d;3%dm", style, color);
        printf("\033[%d;%dH▄█▀▀▀█▄\n", y, x + 1);
        printf("\033[%d;%dH█  ■  █\n", y + 1, x + 1);
        printf("\033[%d;%dH▀█▄▄▄█▀\n", y + 2, x + 1);
    }
}

void printGame()
{
    printMap();
    if (wideness)
    {
        interface(2, 3, 72, 6);
        interface(2, 9, 72, 31);
        interface(75, 3, 30, 37);
    }
    else
    {
        interface(2, 3, 65, 5);
        interface(2, 8, 65, 25);
        interface(68, 3, 30, 30);
    }
    for (int i = 0; i < 7; i++)
    {
        printCell(-1, i, 3, true);
    }
    /*    for (int i = 0; i < 6; i++)
            for (int j = 0; j < 7; j++)
                if (Matrix[i][j] != 0)
                    printCell(i, j, Matrix[i][j]);
    */
}

void setCell(int x, int value)
{
    int y = 0;
    for (int i = 0; i < 6; i++)
        if (Matrix[i][x] == 0)
            y = i;
        else
            break;
    Matrix[y][x] = value;
    printCell(y, x, value, false);
}

int getCellInfo(int x)
{
    int y = 0;
    for (int i = 0; i < 6; i++)
        if (Matrix[i][x] == 0)
        {
            y = 1;
            break;
        }
    return y;
}

void chooseCell(int value)
{
    tcgetattr(STDIN_FILENO, &old_term);
    termios new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
    signal(SIGINT, restorer);

    int current = 3;
    char ch;
    printCell(-1, 3, 2, true);

    while (true)
    {
        if (read(STDIN_FILENO, &ch, 1) > 0)
        {
            bool moved = false;
            if (ch == 27)
            {
                fd_set fds;
                struct timeval tv;
                FD_ZERO(&fds);
                FD_SET(STDIN_FILENO, &fds);
                tv.tv_sec = 0;
                tv.tv_usec = 50000;
                if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0)
                {
                    char seq[2] = {0};
                    read(STDIN_FILENO, &seq[0], 1);
                    if (read(STDIN_FILENO, &seq[1], 1) > 0)
                    {
                        if (seq[0] == '[' || seq[0] == 'O')
                        {
                            if ((seq[1] == 'D') && current > 0)
                            {
                                printCell(-1, current, 3, true);
                                current--;
                                moved = true;
                            }
                            else if ((seq[1] == 'C') && current < 6)
                            {
                                printCell(-1, current, 3, true);
                                current++;
                                moved = true;
                            }
                        }
                    }
                }
            }
            if ((ch == 'A' || ch == 'a') && current > 0)
            {
                printCell(-1, current, 3, true);
                current--;
                moved = true;
            }
            if ((ch == 'D' || ch == 'd') && current < 6)
            {
                printCell(-1, current, 3, true);
                current++;
                moved = true;
            }

            if (moved)
            {
                NetPacket pkt = {0, current};
                send(sockfd, &pkt, sizeof(pkt), MSG_NOSIGNAL);
            }

            if (ch == '\n' || ch == ' ' || ch == 'S' || ch == 's')
            {
                if (getCellInfo(current))
                {
                    printCell(-1, current, 3, true);
                    setCell(current, value);
                    NetPacket pkt = {1, current};
                    send(sockfd, &pkt, sizeof(pkt), MSG_NOSIGNAL);
                    break;
                }
            }
            printCell(-1, current, 2, true);
        }
    }
}

int setup_server()
{
    struct sockaddr_in server, client;
    socklen_t len = sizeof(client);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return -1;
    }
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind");
        return -1;
    }
    cout << "Сервер запущен на порту " << PORT << ". Ожидание подключения..." << endl;
    if (listen(sockfd, 1) < 0)
    {
        perror("listen");
        return -1;
    }
    int client_sock = accept(sockfd, (struct sockaddr *)&client, &len);
    close(sockfd);
    sockfd = client_sock;
    cout << "Игрок подключен! Игра начинается." << endl;
    return 0;
}

int setup_client(const char *ip)
{
    struct sockaddr_in server;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return -1;
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ip, &server.sin_addr) <= 0)
    {
        perror("inet_pton");
        return -1;
    }
    cout << "Подключение к " << ip << ":" << PORT << "..." << endl;
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect");
        return -1;
    }
    cout << "Подключено! Ожидание хода соперника..." << endl;
    return 0;
}

void *net_recv(void *)
{
    NetPacket pkt;
    while (true)
    {
        int bytes_read = 0;
        while (bytes_read < (int)sizeof(pkt))
        {
            int r = recv(sockfd, ((char *)&pkt) + bytes_read, sizeof(pkt) - bytes_read, 0);
            if (r <= 0)
                return NULL;
            bytes_read += r;
        }

        if (pkt.type == 0)
        {
            pthread_mutex_lock(&console_mtx);
            if (last_remote_cursor_col != -1)
                printCell(-1, last_remote_cursor_col, 3, true);
            last_remote_cursor_col = pkt.col;
            printCell(-1, pkt.col, 2, true);
            pthread_mutex_unlock(&console_mtx);
        }
        else if (pkt.type == 1)
        {
            pthread_mutex_lock(&net_mtx);
            remote_col = pkt.col;
            remote_ready = true;
            pthread_cond_signal(&net_cnd);
            pthread_mutex_unlock(&net_mtx);
        }
    }
    return NULL;
}

int checkScreen(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws))
        return -1;
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0;
}

void intro()
{
    printf("\033[2J\033[H");
    string choose;
    cout << "\n\nSet terminal(2) scale?\n";
    cout << "▄█▀▀▀█▄" << endl
         << "█  ■  █" << endl
         << "▀█▄▄▄█▀" << endl
         << endl;
    cout << "▄█▀▀▀▀█▄" << endl
         << "█  ▄▄  █" << endl
         << "█  ▀▀  █" << endl
         << "▀█▄▄▄▄█▀" << endl;
    sleep(1);
    cin >> choose;
    for (char &c : choose)
        c = tolower((unsigned char)c);
    if (choose == "yes" || choose == "y" || choose == "1" || choose == "true")
        wideness = true;
    printf("\033[2J\033[H\033[%d;%dHResize your screen!", 18 + ((int)wideness * 2), 45 + ((int)wideness * 4));
    interface(44 + ((int)wideness * 4), 17 + ((int)wideness * 2), 21, 3);
    printf("\n");
    int rows = 0, cols = 0;
    while (rows < (36 + ((int)wideness * 4)) || cols < (100 + ((int)wideness * 8)))
    {
        checkScreen(&rows, &cols);
        sleep(0.2);
    }
    printf("\033[2J\033[H");
}

int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);
    srand(time(NULL));

    if (argc < 2)
    {
        cout << "Использование:\n  ./game host          (создать сервер)\n  ./game client <IP>   (подключиться к серверу)" << endl;
        return 1;
    }

    pthread_mutex_init(&net_mtx, NULL);
    pthread_cond_init(&net_cnd, NULL);

    if (strcmp(argv[1], "host") == 0)
    {
        if (setup_server() != 0)
            return 1;
        my_turn = true;
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        if (argc < 3)
        {
            cout << "Укажите IP сервера!" << endl;
            return 1;
        }
        if (setup_client(argv[2]) != 0)
            return 1;
        my_turn = false;
    }
    else
    {
        cout << "Неверный параметр. Используйте 'host' или 'client'." << endl;
        return 1;
    }
    intro();
    cout << "\033[2J\033[H\n\n"
         << "\033[?25l\033[?1c";
    init(Matrix);
    int turn = 1;
    map_x = 3;
    map_y = 9;
    printLogMap(Matrix);
    printGame();

    pthread_create(&net_tid, NULL, net_recv, NULL);

    while (true)
    {
        if (my_turn)
        {
            pthread_mutex_lock(&console_mtx);
            if (last_remote_cursor_col != -1)
            {
                printCell(-1, last_remote_cursor_col, 3, true);
                last_remote_cursor_col = -1;
            }
            pthread_mutex_unlock(&console_mtx);

            chooseCell(turn);
            printLogMap(Matrix);
        }
        else
        {
            pthread_mutex_lock(&net_mtx);
            while (!remote_ready)
            {
                pthread_cond_wait(&net_cnd, &net_mtx);
            }
            int col = remote_col;
            remote_ready = false;
            pthread_mutex_unlock(&net_mtx);

            setCell(col, turn);
            printLogMap(Matrix);
        }

        if (!checkGame())
            break;

        turn *= -1;
        my_turn = !my_turn;
    }

    cout << "\033[2J\033[H\n\033[?25h\033[?8c";
    cout << "Игра завершена!" << endl;

    close(sockfd);
    pthread_cancel(net_tid);
    pthread_join(net_tid, NULL);
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    pthread_mutex_destroy(&net_mtx);
    pthread_cond_destroy(&net_cnd);
    pthread_mutex_destroy(&console_mtx);

    return 0;
}