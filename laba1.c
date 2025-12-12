#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_not_full  = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;

bool buffer_has_data = false;

void* producer_thread(void* arg) {
    int total_items = *(int*)arg;
    for (int i = 0; i < total_items; ++i) {
        sleep(1);

        pthread_mutex_lock(&buffer_mutex);

        while (buffer_has_data) {
            pthread_cond_wait(&buffer_not_full, &buffer_mutex);
        }

        buffer_has_data = true;
        printf("provided\n");
        pthread_cond_signal(&buffer_not_empty);
        pthread_mutex_unlock(&buffer_mutex);
    }
    return NULL;
}

void* consumer_thread(void* arg) {
    int total_items = *(int*)arg;
    for (int i = 0; i < total_items; ++i) {
        pthread_mutex_lock(&buffer_mutex);

        while (!buffer_has_data) {
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
        }

        buffer_has_data = false;
        printf("consumed\n");
        pthread_cond_signal(&buffer_not_full);
        pthread_mutex_unlock(&buffer_mutex);
    }
    return NULL;
}

int main() {
    const int NUM_OPERATIONS = 5;
    pthread_t producer_tid, consumer_tid;

    pthread_create(&producer_tid, NULL, producer_thread, &NUM_OPERATIONS);
    pthread_create(&consumer_tid, NULL, consumer_thread, &NUM_OPERATIONS);

    pthread_join(producer_tid, NULL);
    pthread_join(consumer_tid, NULL);

    pthread_mutex_destroy(&buffer_mutex);
    pthread_cond_destroy(&buffer_not_full);
    pthread_cond_destroy(&buffer_not_empty);

    return 0;
}