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
const char *NAME_MAX_FILE = "Output_Max.txt";
volatile int max_int = INT_MIN;

// Synchronisation variables
bool should_work = true;
pthread_mutex_t MUTEX_OUTPUT_FILE;
pthread_mutex_t MUTEX_MAX_FILE;
pthread_mutex_t MUTEX_DEV_URANDOM;
pthread_cond_t CV_OUTPUT_FILE;
pthread_cond_t CV_MAX_FILE;
pthread_cond_t CV_DEV_URANDOM;

// Function declarations
void *thread_fill_task();
void *thread_fill_file_task();
void *fill_memory();
int *perform_mmap(void *from, size_t size);
void *thread_count_max_task();

int main()
{
    pthread_mutex_init(&MUTEX_OUTPUT_FILE, NULL);
    pthread_mutex_init(&MUTEX_MAX_FILE, NULL);
    pthread_mutex_init(&MUTEX_DEV_URANDOM, NULL);
    pthread_cond_init(&CV_OUTPUT_FILE, NULL);
    pthread_cond_init(&CV_MAX_FILE, NULL);
    pthread_cond_init(&CV_DEV_URANDOM, NULL);

    printf("-> Before allocation\n");
    getchar();

    DEV_URANDOM = fopen("/dev/urandom", "r");
    int *mmap_pointer = perform_mmap(MMAP_ADDRESS, MMAP_SIZE);
    if (mmap_pointer == MAP_FAILED)
        return -1;

    printf("-> After allocation\n");
    getchar();

    pthread_t fillmemory_threads[NUM_MMAP_THREADS];
    for (int i = 0; i < NUM_MMAP_THREADS; i++)
    {
        int return_code = pthread_create(&fillmemory_threads[i], NULL, thread_fill_task, NULL);
        if (return_code)
            printf("Thread failed with return code %d\n", return_code);
    }
    printf("-> Threads are filling memory\n");
    getchar();
    int munmap_status = munmap(mmap_pointer, MMAP_SIZE);
    printf("-> After deallocation\n");
    getchar();
    mmap_pointer = perform_mmap(MMAP_ADDRESS, MMAP_SIZE);

    pthread_t garbage_refresher_thread;
    pthread_create(&garbage_refresher_thread, NULL, thread_fill_file_task, NULL);

    printf("-> Now let's find max!\n");

    pthread_t readfile_threads[NUM_READFILE_THREADS];
    for (size_t i = 0; i < NUM_READFILE_THREADS; i++)
    {
        int return_code = pthread_create(&readfile_threads[i], NULL, thread_count_max_task, NULL);
        if (return_code)
            printf("Thread failed with return code %d\n", return_code);
    }

    pthread_join(garbage_refresher_thread, NULL);

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
    munmap_status = munmap(mmap_pointer, MMAP_SIZE);
    if (munmap_status != 0)
    {
        printf("Unmapping Failed\n");
        return 1;
    }
    printf("-> After deallocation\n");
    getchar();

    return 0;
}

void *fill_memory()
{
    pthread_mutex_lock(&MUTEX_DEV_URANDOM);
    pthread_cond_wait(&CV_DEV_URANDOM, &MUTEX_DEV_URANDOM);
    fread(MMAP_ADDRESS, MMAP_SIZE, 1, DEV_URANDOM);
    pthread_cond_broadcast(&CV_DEV_URANDOM);
    pthread_mutex_unlock(&MUTEX_DEV_URANDOM);
    return NULL;
}

void *thread_fill_task()
{
    while (should_work)
    {
        fill_memory();
    }
    pthread_cond_broadcast(&CV_DEV_URANDOM);
    pthread_mutex_unlock(&MUTEX_DEV_URANDOM);
    pthread_exit(NULL);
    return NULL;
}

void *thread_count_max_task()
{
    while (should_work)
    {
        pthread_mutex_lock(&MUTEX_OUTPUT_FILE);
        pthread_cond_wait(&CV_OUTPUT_FILE, &MUTEX_OUTPUT_FILE);
        FILE *file = fopen(NAME_OUTPUT_FILE, "r");
        fseek(file, 0, 0);
        int number = INT_MIN;
        while (!feof(file))
        {
            fread(&number, sizeof(int), 1, file);
            fflush(file);
            if (number > max_int)
            {
                pthread_mutex_lock(&MUTEX_MAX_FILE);
                pthread_cond_wait(&CV_MAX_FILE, &MUTEX_MAX_FILE);
                FILE *max_file = fopen(NAME_MAX_FILE, "a");
                fprintf(max_file, "Current max is %d\n", number);
                fclose(max_file);
                max_int = number;
                pthread_cond_broadcast(&CV_MAX_FILE);
                pthread_mutex_unlock(&MUTEX_MAX_FILE);
            }
        }
        pthread_mutex_unlock(&MUTEX_OUTPUT_FILE);
        fclose(file);
    }
    pthread_mutex_unlock(&MUTEX_OUTPUT_FILE);
    pthread_exit(NULL);
    return NULL;
}

void *thread_fill_file_task()
{
    while (should_work)
    {
        pthread_mutex_lock(&MUTEX_OUTPUT_FILE);
        FILE *file = fopen(NAME_OUTPUT_FILE, "a");
        srandom(time(NULL)); // Seed randomizer
        int num_io = OUTPUT_FILE_SIZE / IO_BLOCK_SIZE;
        int size_offset = MMAP_SIZE / IO_BLOCK_SIZE;
        for (int i = 0; i < 2 * num_io; i++)
        {
            int address_offset = random() % size_offset;
            void *address = (void *)((int *)MMAP_ADDRESS + address_offset);
            int file_offset = random() & num_io;

            fseek(file, file_offset, 0);
            fwrite(address, IO_BLOCK_SIZE, 1, file);
        }
        pthread_cond_broadcast(&CV_OUTPUT_FILE);
        pthread_mutex_unlock(&MUTEX_OUTPUT_FILE);
        fclose(file);
    }
    pthread_cond_broadcast(&CV_OUTPUT_FILE);
    pthread_mutex_unlock(&MUTEX_OUTPUT_FILE);
    pthread_exit(NULL);
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