#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>

// Allocation data

FILE *DEV_URANDOM;
void *MMAP_ADDRESS = (void *)0xF6A3975;
const int MMAP_SIZE = 33 * 1024 * 1024;
const int NUM_MMAP_THREADS = 66;

// File data
FILE *OUTPUT_FILE;
const int OUTPUT_FILE_SIZE = 190 * 1024 * 1024;
const int IO_BLOCK_SIZE = 114;
const int NUM_READFILE_THREADS = 107;
const char *NAME_OUTPUT_FILE = "Output.txt";

volatile int max_int = INT_MIN;

// Function declarations
void *thread_fill_task();
void *fill_memory();
int *perform_mmap(void *from, size_t size);
void *thread_count_max_task();

int main()
{
    printf("-> Before allocation\n");

    DEV_URANDOM = fopen("/dev/urandom", "r");
    int *mmap_pointer = perform_mmap(MMAP_ADDRESS, MMAP_SIZE);
    if (mmap_pointer == MAP_FAILED)
        return -1;

    printf("-> After allocation\n");

    pthread_t fillmemory_threads[NUM_MMAP_THREADS];
    for (int i = 0; i < NUM_MMAP_THREADS; i++)
    {
        int return_code = pthread_create(&fillmemory_threads[i], NULL, thread_fill_task, NULL);
        if (return_code)
            printf("Thread failed with return code %d\n", return_code);
    }

    printf("-> Threads are running\n");

    printf("-> Before file io\n");

    printf("-> Let's continue filling the file with garbage!\n");

    printf("-> Now let's find max!\n");

    pthread_t readfile_threads[NUM_READFILE_THREADS];
    for (size_t i = 0; i < NUM_READFILE_THREADS; i++)
    {
        int return_code = pthread_create(&readfile_threads[i], NULL, thread_count_max_task, NULL);
        if (return_code)
            printf("Thread failed with return code %d\n", return_code);
    }

    for (int i = 0; i < NUM_MMAP_THREADS; i++)
    {
        pthread_join(fillmemory_threads[i], NULL);
    }

    for (int i = 0; i < NUM_READFILE_THREADS; i++)
    {
        pthread_join(readfile_threads[i], NULL);
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
    return NULL;
}

void *thread_fill_task()
{
    while (true)
    {
        fill_memory();
    }
    pthread_exit(NULL);
    return NULL;
}

void *thread_count_max_task()
{
    while (true)
    {
        FILE *file = fopen(NAME_OUTPUT_FILE, "r");
        /*
        int number = INT_MIN;
        while (!feof(file))
        {
            fread(&number, sizeof(int), 1, OUTPUT_FILE);
            if (number > max_int)
            {
                FILE *max_file = fopen("current_max.txt", "w");
                fprintf(max_file, "Current max is %d", number);
                fclose(max_file);
                max_int = number;
            }
        }
        */
        fclose(file);
    }
    return NULL;
}

void *thread_fill_file_task()
{
    while (true)
    {
        OUTPUT_FILE = fopen(NAME_OUTPUT_FILE, "a");
        srandom(time(NULL)); // Seed randomizer
        int num_io = OUTPUT_FILE_SIZE / IO_BLOCK_SIZE;
        int size_offset = MMAP_SIZE / IO_BLOCK_SIZE;
        for (int i = 0; i < 2 * num_io; i++)
        {
            int address_offset = random() % size_offset;
            void *address = (void *)((int *)MMAP_ADDRESS + address_offset);
            int file_offset = random() & num_io;
            fseek(OUTPUT_FILE, file_offset, 0);
            fwrite(address, IO_BLOCK_SIZE, 1, OUTPUT_FILE);
        }
        fclose(OUTPUT_FILE);
    }
    return NULL;
}

int *perform_mmap(void *from, size_t size)
{
    // TODO Google map flags
    int *mmap_status = mmap(from, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mmap_status == MAP_FAILED)
        exit(1);
    return mmap_status;
}