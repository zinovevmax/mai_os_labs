#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define FILEPATH "tmp/file.txt"
#define FILESIZE 64

// Обработчик сигнала, необходимый для дочернего процесса
void signal_handler(int sig) {
    // Ничего не делаем; просто ожидаем сигнала
}

int main() {
    char filename[100];
    // Открываем файл с правами на чтение и запись
    int fd  = open(FILEPATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Error opening file.");
        exit(1);
    }

    // Устанавливаем размер файла
    if (ftruncate(fd, FILESIZE) == -1) {
        perror("Error setting file size.");
        close(fd);
        exit(1);
    }

    // Отображаем файл
    char* memptr  = (char*)mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (memptr == MAP_FAILED) {
        perror("Error mapping file.");
        close(fd);
        exit(1);
    }
    close(fd);

    // Создаём дочерний процесс
    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("Fork failed.");
        munmap(memptr, FILESIZE);
        exit(1);
    }
    
    if (child_pid == 0) {
        // Дочерний процесс: установка обработчика для SIGUSR2
        signal(SIGUSR1, signal_handler);
        
        // Ожидаем сигнал от родительского процесса
        pause();

        // Запускаем дочерний процесс через execve
        char filesize_str[10];
        sprintf(filesize_str, "%d", FILESIZE);
        const char* args[] = {"./child", FILEPATH, filesize_str, NULL};

        execve("./child", (char* const*)args, NULL); 
        perror("execve failed.");
        exit(1);
    } else {
        // Родительский процесс
        printf("Введите имя файла: ");
        scanf("%s", filename);
        FILE* file = fopen(filename, "r");
        if (file == NULL) {
            perror("Не удалось открыть файл");
            return 1;   
        }

        int number;
        char* buffer = memptr;
        while(fscanf(file, "%d\n", &number) != -1)
        {
            // отправляем его во второй процесс через memptr
            buffer += sprintf(buffer, "%d ", number);
        }
        fclose(file);

        // Отправляем сигнал SIGUSR2 дочернему процессу
        kill(child_pid, SIGUSR1);

        // Ожидаем завершения дочернего процесса
        wait(NULL);

        // Освобождаем память
        munmap(memptr, FILESIZE);

        return 0;
    } 
}