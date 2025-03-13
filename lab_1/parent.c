#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define READ_END 0
#define WRITE_END 1

int main() {
    char filename[100];
    pid_t pid;
    int fd[2];  // Pipe для связи между процессами

    // Запрашиваем имя файла у пользователя
    printf("Введите имя файла: ");
    scanf("%s", filename);

    // Создаем pipe
    if (pipe(fd) == -1) {
        perror("Ошибка создания pipe");
        return 1;
    }

    pid = fork();
    if (pid < 0) {
        perror("Ошибка вызова fork");
        close(fd[READ_END]);
        close(fd[WRITE_END]);
        return 1;
    }
    
    if (pid == 0) {
        // Дочерний процесс

        // Закрываем ненужную часть pipe
        close(fd[WRITE_END]);

        char pipe_read_fd[10];
        sprintf(pipe_read_fd, "%d", fd[0]);

        // Аргументы для execve
        char *args[] = {"./child", pipe_read_fd, NULL};

        // Выполняем дочернюю программу
        execve("./child", args, NULL);

        // Это сообщение выведется, если exec потерпел неудачу
        perror("execve");
        return 1;

    } else {
        // Родительский процесс

        // Закрываем ненужную часть pipe
        close(fd[READ_END]);

        // Открытие файла для чтения
        FILE* file = fopen(filename, "r");
        if (file == NULL) {
            perror("Не удалось открыть файл");
            close(fd[READ_END]);
            close(fd[WRITE_END]);
            return 1;   
        }

        // Читаем числа из файла
        int number;
        char buffer[32];
        while (fscanf(file, "%d\n", &number) > 0) {
            sprintf(buffer, "%d", number);

            // Пишем числа в pipe
            write(fd[WRITE_END], buffer, 32);
        }

        close(fd[WRITE_END]);
        wait(NULL);  // Ждем завершения дочернего процесса
    }  

    return 0;
}
