#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <chrono>
#include <iostream>


#define MAX_THREADS 100

typedef struct {
    int id;
    int k;
    int window_size;
    double **matrix;
    double **result_matrix;
    int matrix_size;
    pthread_mutex_t *mutex;
} ThreadData;

void* apply_filter(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int i, j, x, y;
    double sum;
    int half_window = data->window_size / 2;

    for (i = data->id; i < data->matrix_size; i += data->k) {
        for (j = 0; j < data->matrix_size; j++) {
            sum = 0.0;
            for (x = -half_window; x <= half_window; x++) {
                for (y = -half_window; y <= half_window; y++) {
                    int xi = i + x;
                    int yj = j + y;
                    if (xi >= 0 && xi < data->matrix_size && yj >= 0 && yj < data->matrix_size) {
                        sum += data->matrix[xi][yj];
                    }
                }
            }
            pthread_mutex_lock(data->mutex);
            data->result_matrix[i][j] = sum / (data->window_size * data->window_size);
            pthread_mutex_unlock(data->mutex);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <matrix_size> <window_size> <max_threads> <k_times>\n", argv[0]);
        exit(1);
    }

    int matrix_size = atoi(argv[1]);
    int window_size = atoi(argv[2]);
    int max_threads = atoi(argv[3]);
    int k_times = atoi(argv[4]);

    if (max_threads > MAX_THREADS) {
        fprintf(stderr, "Maximum number of threads exceeded. Maximum is %d\n", MAX_THREADS);
        exit(1);
    }

    double **matrix = (double **)malloc(matrix_size * sizeof(double *));
    double **result_matrix = (double **)malloc(matrix_size * sizeof(double *));
    for (int i = 0; i < matrix_size; i++) {
        matrix[i] = (double *)malloc(matrix_size * sizeof(double));
        result_matrix[i] = (double *)malloc(matrix_size * sizeof(double));
        for (int j = 0; j < matrix_size; j++) {
            matrix[i][j] = (double)(rand() % 100);
            result_matrix[i][j] = 0.0;
        }
    }

    pthread_t threads[max_threads];
    ThreadData thread_data[max_threads];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    for (int k = 0; k < k_times; k++) {
        for (int i = 0; i < max_threads; i++) {
            thread_data[i].id = i;
            thread_data[i].k = max_threads;
            thread_data[i].window_size = window_size;
            thread_data[i].matrix = matrix;
            thread_data[i].result_matrix = result_matrix;
            thread_data[i].matrix_size = matrix_size;
            thread_data[i].mutex = &mutex;
            pthread_create(&threads[i], NULL, apply_filter, (void *)&thread_data[i]);
        }

        for (int i = 0; i < max_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        // После завершения фильтрации, результат становится исходной матрицей для следующего применения
        for (int i = 0; i < matrix_size; i++) {
            for (int j = 0; j < matrix_size; j++) {
                matrix[i][j] = result_matrix[i][j];
                result_matrix[i][j] = 0.0; // Сброс результата для следующего применения
            
            }
    
        }
        
    }

    // for (int i = 0; i < matrix_size; i++) {
    //     for (int j = 0; j < matrix_size; j++) {
    //         printf("%.2f ", matrix[i][j]);
    //     }
    //     printf("\n");
    // }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[µs]" << std::endl;

    for (int i = 0; i < matrix_size; i++) {
        free(matrix[i]);
        free(result_matrix[i]);
    }
    free(matrix);
    free(result_matrix);

    return 0;
}


