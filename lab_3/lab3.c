#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/wait.h>

#define BUFFER_SIZE 256

// Флаг для сигнализации о завершении
volatile sig_atomic_t should_exit = 0;

// Обработчик сигнала SIGUSR1
void handle_signal(int sig) {
    should_exit = 1;
}

// Функция для проверки числа на простоту
int is_prime(int num) {
    if (num <= 1) return 0; // Не является простым
    for (int i = 2; i * i <= num; i++) {
        if (num % i == 0) return 0; // Составное
    }
    return 1; // Простое
}

int main() {
    char filename[BUFFER_SIZE];
    printf("Введите имя файла: ");
    scanf("%s", filename);

    // Создание разделяемой памяти
    int fd = shm_open("/shared_memory", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("Ошибка создания отображаемого файла");
        return 1;
    }
    ftruncate(fd, sizeof(int));

    int *shared_memory = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("Ошибка отображения памяти");
        close(fd);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Ошибка создания процесса");
        return 1;
    }

    if (pid == 0) { // Дочерний процесс
        signal(SIGUSR1, handle_signal);

        FILE *file = fopen(filename, "r");
        if (!file) {
            perror("Ошибка открытия файла");
            exit(1);
        }

        int number;
        while (fscanf(file, "%d", &number) == 1) {
            if (number < 0 || is_prime(number)) {
                // Если число отрицательное или простое, завершаем работу
                shared_memory[0] = number;
                kill(getppid(), SIGUSR1); // Сигнализируем родителю
                fclose(file);
                exit(0);
            } else {
                // Если число составное, выводим его
                printf("%d\n", number);
            }
        }

        fclose(file);
        exit(0);
    } else { // Родительский процесс
        struct sigaction sa_usr1 = {0};
        sa_usr1.sa_handler = handle_signal;
        sigaction(SIGUSR1, &sa_usr1, NULL);

        while (!should_exit) {
            pause(); // Ждем сигнала
        }

        // Ожидание завершения дочернего процесса
        wait(NULL);

        // Освобождение ресурсов
        munmap(shared_memory, sizeof(int));
        shm_unlink("/shared_memory");
    }

    return 0;
}