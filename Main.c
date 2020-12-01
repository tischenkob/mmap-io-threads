#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <time.h>
#include <stdbool.h>

// Allocation data

FILE *DEV_URANDOM;
void *MMAP_ADDRESS = (void *)0xF6A3975;
const int MMAP_SIZE = 33 * 1024 * 1024;
const int NUM_MMAP_THREADS = 66;

void *thread_fill_task();
void *fill_memory();
int *perform_mmap(void *from, size_t size);

// File data
const int WRITE_FILE_SIZE = 190 * 1024 * 1024;
const int IO_BLOCK_SIZE = 114;
const int NUM_READFILE_THREADS = 107;
const char *NAME_OUTPUT_FILE = "Output.txt";

int main()
{
    printf("-> Before allocation\n");
    DEV_URANDOM = fopen("/dev/urandom", "r");

    // Allocation
    int *mmap_pointer = perform_mmap(MMAP_ADDRESS, MMAP_SIZE);
    if (mmap_pointer == MAP_FAILED)
        return -1;

    printf("-> After allocation\n");

    // Run threads
    pthread_t threads[NUM_MMAP_THREADS];
    for (int i = 0; i < NUM_MMAP_THREADS; i++)
    {
        int return_code = pthread_create(&threads[i], NULL, thread_fill_task, NULL);
        if (return_code)
            printf("Thread failed with return code %d\n", return_code);
    }
    printf("-> Threads are running\n");
    for (int i = 0; i < NUM_MMAP_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // FILE I/O
    printf("-> Before file io\n");
    FILE *output_file = fopen(NAME_OUTPUT_FILE, "w");
    srandom(time(NULL)); // Seed randomizer

    int num_io = WRITE_FILE_SIZE / IO_BLOCK_SIZE;
    int size_offset = MMAP_SIZE / IO_BLOCK_SIZE;
    for (int i = 0; i < num_io; i++)
    {
        int offset = random() % size_offset;
        void *address = (void *)((int *)MMAP_ADDRESS + offset);
        fwrite(address, IO_BLOCK_SIZE, 1, output_file);
    }

    fclose(DEV_URANDOM);
    fclose(output_file);

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
    return NULL;
}

void *thread_fill_task()
{
    fill_memory();
    pthread_exit(NULL);
    return NULL;
}

int *perform_mmap(void *from, size_t size)
{
    // TODO Google map flags
    int *mmap_status = mmap(from, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mmap_status == MAP_FAILED)
    {
        printf("Mapping at the given address failed\n");
        mmap_status = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (mmap_status == MAP_FAILED)
        {
            printf("Mapping failed\n");
        }
    }
    return mmap_status;
}