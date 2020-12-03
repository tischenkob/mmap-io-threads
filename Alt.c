#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>

// Allocation data
void *MMAP_ADDRESS = (void *)0xF6A3975;
const int MMAP_SIZE = 33 * 1024 * 1024;
const int NUM_MMAP_THREADS = 66;

// File data
const int NUM_FILLFILE_THREADS = 1;
const int OUTPUT_FILE_SIZE = 190 * 1024 * 1024;
const int IO_BLOCK_SIZE = 114;
const int NUM_READFILE_THREADS = 107;
const char *NAME_OUTPUT_FILE = "Output.txt";
const char *NAME_MAX_FILE = "Output_Max.txt";
volatile int max_int = INT_MIN;

// Synchronisation variables
bool should_work = true;
pthread_mutex_t MUTEX_OUTPUT_FILE;
pthread_cond_t CV_OUTPUT_FILE;

// Function declarations
void spawn_threads(pthread_t *threads, int amount, void *(*function)(void *));
void join_threads(int amount, pthread_t *ptr);
void *fill_memory();
void *fill_file();
void *read_file();

int main()
{
    printf("-> before mmap\n");
    getchar();
    // Setup
    pthread_mutex_init(&MUTEX_OUTPUT_FILE, NULL);
    pthread_cond_init(&CV_OUTPUT_FILE, NULL);
    int *mmap_status = mmap(MMAP_ADDRESS, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    printf("-> after mmap\n");
    getchar();
    // Fill memory
    pthread_t threads_mem[NUM_MMAP_THREADS];
    spawn_threads(threads_mem, NUM_MMAP_THREADS, fill_memory);
    pthread_t threads_filew[NUM_FILLFILE_THREADS];
    spawn_threads(threads_filew, NUM_FILLFILE_THREADS, fill_file);
    pthread_t threads_filer[NUM_READFILE_THREADS];
    spawn_threads(threads_filer, NUM_READFILE_THREADS, read_file);
    printf("-> after filling mmap\n");
    getchar();
    should_work = false;
    printf("-> joining threads\n");
    join_threads(NUM_MMAP_THREADS, threads_mem);
    printf("-> joined memory threads\n");
    join_threads(NUM_FILLFILE_THREADS, threads_filew);
    printf("-> joined file filling threads\n");
    join_threads(NUM_READFILE_THREADS, threads_filer);
    printf("-> joined file reading threads\n");
    int munmap_status = munmap(mmap_status, MMAP_SIZE);
    printf("-> after munmap\n");
    getchar();
    return 0;
}

void spawn_threads(pthread_t *threads, int amount, void *(*function)(void *))
{
    for (int i = 0; i < amount; i++)
        pthread_create(&threads[i], NULL, function, NULL);
}
void join_threads(int amount, pthread_t *ptr)
{
    for (int i = 0; i < amount; i++)
        pthread_join(*(ptr + i), NULL);
}

void *fill_memory()
{
    while (should_work)
    {
        FILE *urandom = fopen("/dev/urandom", "r");
        fread(MMAP_ADDRESS, MMAP_SIZE, 1, urandom);
        fclose(urandom);
    }
    pthread_exit(NULL);
}

void *fill_file()
{
    while (should_work)
    {
        pthread_mutex_lock(&MUTEX_OUTPUT_FILE);
        FILE *output = fopen(NAME_OUTPUT_FILE, "w");
        srandom(time(NULL));
        int num_io = OUTPUT_FILE_SIZE / IO_BLOCK_SIZE;
        int size_offset = MMAP_SIZE / IO_BLOCK_SIZE;
        for (int i = 0; i < num_io; i++)
        {
            int address_offset = random() % size_offset;
            void *address = (void *)((int *)MMAP_ADDRESS + address_offset);
            fwrite(address, IO_BLOCK_SIZE, 1, output);
        }
        fclose(output);
        pthread_cond_broadcast(&CV_OUTPUT_FILE);
        pthread_mutex_unlock(&MUTEX_OUTPUT_FILE);
    }
    pthread_exit(NULL);
    return NULL;
}

void *read_file()
{
    while (should_work)
    {
        pthread_mutex_lock(&MUTEX_OUTPUT_FILE);
        pthread_cond_wait(&CV_OUTPUT_FILE, &MUTEX_OUTPUT_FILE);
        FILE *file = fopen(NAME_OUTPUT_FILE, "r");
        int number = INT_MIN;
        while (!feof(file))
        {
            if (!should_work)
                break;
            fread(&number, sizeof(int), 1, file);
            if (number > max_int)
            {
                FILE *max_file = fopen(NAME_MAX_FILE, "a");
                fprintf(max_file, "Current max is %d\n", number);
                fclose(max_file);
                max_int = number;
            }
        }
        fclose(file);
        pthread_cond_broadcast(&CV_OUTPUT_FILE);
        pthread_mutex_unlock(&MUTEX_OUTPUT_FILE);
    }
    pthread_exit(NULL);
    return NULL;
}