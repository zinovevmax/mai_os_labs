#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

int is_prime(int n) {
    if (n <= 1)
        return 0;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0)
            return 0;
    }
    return 1;
}

// #define FILEPATH "/tmp/file"
// #define FILESIZE 10

// Обработчик сигнала, необходимый для дочернего процесса
void signal_handler(int sig) {
    // Ничего не делаем; просто ожидаем сигнала
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <parent_pid>\n", argv[0]);
        return 1;
    }

    // Получаем аргументы

    // Открываем файл и отображаем его в память
    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("Error opening file in child.");
        return 1;
    }

    int FILESIZE = atoi(argv[2]);

    char* memptr = (char*)mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (memptr == MAP_FAILED) {
        perror("Error mapping file in child.");
        close(fd);
        return 1;
    }
    close(fd);

    char *token = strtok(memptr, " ");
    while (token != NULL) {
        int number = atoi(token);
        if (number <= 1 || is_prime(number)) {
        } else {
            printf("%d\n", number);
        }
        token = strtok(NULL, " ");
    }

    // Освобождаем память
    munmap(memptr, FILESIZE);
    return 0;
}