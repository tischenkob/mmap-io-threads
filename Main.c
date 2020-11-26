#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>

const FILE *DEV_URANDOM;
void *MMAP_ADDRESS = (void *)0xF6A3975;
const long long int MMAP_SIZE = 33 * 1024 * 1024; //33 mb
const int NUM_MMAP_THREADS = 66;

int thread_counter = 0;
void *fill_memory();
int *perform_mmap();

int main()
{
    DEV_URANDOM = fopen("/dev/urandom", "r");
    int *mmap_pointer = perform_mmap();
    if (mmap_pointer == MAP_FAILED)
        return -1;

    pthread_t threads[NUM_MMAP_THREADS];
    for (int i = 0; i < NUM_MMAP_THREADS; i++)
    {
        int return_code = pthread_create(&threads[i], NULL, fill_memory, NULL);
        if (return_code)
        {
            printf("Thread failed with return code %d", return_code);
        }
    }

    int munmap_status = munmap(mmap_pointer, MMAP_SIZE);
    if (munmap_status != 0)
    {
        printf("Unmapping Failed\n");
        return 1;
    }

    pthread_exit(NULL);
    return 0;
}

void *fill_memory()
{
    printf("Hello! I'm thread number %d \n", thread_counter++);
    pthread_exit(NULL);
}

int *perform_mmap()
{
    int *mmap_status = mmap(MMAP_ADDRESS, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mmap_status == MAP_FAILED)
    {
        printf("Mapping at the given address failed\n");
        mmap_status = mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (mmap_status == MAP_FAILED)
        {
            printf("Mapping failed\n");
        }
    }
    return mmap_status;
}