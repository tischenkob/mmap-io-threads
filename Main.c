#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>

FILE *DEV_URANDOM;
void *MMAP_ADDRESS = (void *)0xF6A3975;
const long long int MMAP_SIZE = 33 * 1024 * 1024; //33 mb
const int NUM_MMAP_THREADS = 66;

int thread_counter = 0;
void *fill_memory();
int *perform_mmap();

int main()
{
    DEV_URANDOM = fopen("/dev/urandom", "r");

    // Allocation
    int *mmap_pointer = perform_mmap();
    if (mmap_pointer == MAP_FAILED)
        return -1;

    // Run threads
    pthread_t threads[NUM_MMAP_THREADS];
    for (int i = 0; i < NUM_MMAP_THREADS; i++)
    {
        int return_code = pthread_create(&threads[i], NULL, fill_memory, NULL);
        if (return_code)
            printf("Thread failed with return code %d", return_code);
    }

    for (int i = 0; i < NUM_MMAP_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    fclose(DEV_URANDOM);

    // Deallocation
    int munmap_status = munmap(mmap_pointer, MMAP_SIZE);
    if (munmap_status != 0)
    {
        printf("Unmapping Failed\n");
        return 1;
    }

    return 0;
}

void *fill_memory()
{
    fread(MMAP_ADDRESS, MMAP_SIZE, 1, DEV_URANDOM);
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