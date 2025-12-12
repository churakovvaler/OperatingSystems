#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
bool ready = false;

void* provider(void* arg) {
    int count = *(int*)arg;
    for (int i = 0; i < count; ++i) {
        sleep(1);
        pthread_mutex_lock(&lock);
        while (ready) {
            pthread_cond_wait(&cond, &lock);
        }
        ready = true;
        printf("provided\n");
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void* consumer(void* arg) {
    int count = *(int*)arg;
    for (int i = 0; i < count; ++i) {
        pthread_mutex_lock(&lock);
        while (!ready) {
            pthread_cond_wait(&cond, &lock);
        }
        ready = false;
        printf("consumed\n");
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    const int N = 5;
    pthread_t prov_thr, cons_thr;

    int count = N;

    pthread_create(&prov_thr, NULL, provider, &count);
    pthread_create(&cons_thr, NULL, consumer, &count);

    pthread_join(prov_thr, NULL);
    pthread_join(cons_thr, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    return 0;
}