#include <model_calcer_wrapper.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>

int memoryusage() {
    char filename[1000];
    sprintf(filename, "/proc/%d/stat", getpid());
    FILE *f = fopen(filename, "r");

    char unused[1000];
    for (int i = 0; i < 23; i++) {
        fscanf(f, "%s", &unused);
    }

    int mem;
    fscanf(f, "%d", &mem);
    
    fclose(f);

	return (mem * 4096) / 1024 / 1024;
}

ModelCalcerHandle* model = NULL;
int lastMemoryUsage = 0;
volatile int threadToLoadModel = 0;
volatile int nextID = 0;
#define threadCount 128

void gorountine() {
    int myID = __atomic_fetch_add(&nextID, 1, __ATOMIC_SEQ_CST);

    while (1) {
        if (atomic_load(&threadToLoadModel) == myID) {
            if (model) {
                ModelCalcerDelete(model);
            }

            model = ModelCalcerCreate();
            if (!model) {
                printf("panic: %s\n", GetErrorString());
                return;
            }

            if (!LoadFullModelFromFile(model, "model.bin")) {
                printf("panic: %s\n", GetErrorString());
                return;
            }

            int newMemoryUsage = memoryusage();
            printf("new model loaded on thread: %d, memory usage: %d MB (diff: %d MB)\n", myID, newMemoryUsage, newMemoryUsage-lastMemoryUsage);
            lastMemoryUsage = newMemoryUsage;
            fflush(stdout);

            // Removing this line will remove the memory leak as it always makes the model be loaded by the first thread.
            atomic_store(&threadToLoadModel, rand() % threadCount);
        }

        usleep(100000);
    }
}

void main() {
    pthread_t threads[threadCount];
    for (int i = 0; i < threadCount-1; i++) {
        pthread_create(&threads[i], NULL, gorountine, NULL);
    }

    gorountine();
}
