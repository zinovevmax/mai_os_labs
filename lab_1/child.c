#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// Функция для проверки числа на простоту
int is_prime(int n) {
    if (n <= 1)
        return 0;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0)
            return 0;
    }
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }

    int pipe_fd = atoi(argv[1]);

    // Читаем числа из pipe
    char buffer[32];
    while(read(pipe_fd, buffer, 32)) {
        int number = atoi(buffer);
        // Проверка на отрицательность и простоту
        if (number <= 1 || is_prime(number)) {
        } else {
            printf("%d\n", number);
        }
    }

    close(pipe_fd);
    
    return 0;
}