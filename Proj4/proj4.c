#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

// Prototypes
bool checkTextFile(char *filename);
void serial_architecture();
void *fileProcessingFunction(void *arg);
void readFiles();

// Global Variables
pthread_t *threads;
pthread_mutex_t mutex;

// Global Counters
int badFiles = 0;
int directories = 0;
int regularFiles = 0;
int specialFiles = 0;
int totalRegularFileBytes = 0;
int textRegularFiles = 0;
int totalTextRegularFileBytes = 0;

int main(int argc, char *argv[])
{

   int threadLimit;
   int activeThreads = 0;
   int totalThreads = 0;

   if (argc > 1 && strcmp(argv[1], "thread") == 0)
   {
      // Allocate memory for the thead limit
      if (argv[2] == NULL)
      {
         perror("Please enter a max number of threads");
         exit(1);
      }
      threadLimit = atoi(argv[2]);
      threads = (pthread_t *)malloc(threadLimit * sizeof(pthread_t));

      // Initialize mutex
      if (pthread_mutex_init(&mutex, NULL) != 0)
      {
         perror("Mutex failed");
         exit(1);
      }

      // Reads files and creates thread for files
      char filename[256];
      char *fileDup;
      while (scanf("%s", filename) != EOF)
      {
         if (totalThreads < threadLimit)
         {
            fileDup = strdup(filename);
            pthread_create(&threads[activeThreads], NULL, fileProcessingFunction, (void *)fileDup);
            activeThreads++;
            totalThreads++;
         }
         else
         {
            pthread_join(threads[0], NULL);
            activeThreads--;

            fileDup = strdup(filename);
            pthread_create(&threads[activeThreads], NULL, fileProcessingFunction, (void *)fileDup);
            activeThreads++;
            totalThreads++;
         }
      }

      // Wait for threads to complete
      for (int i = 0; i < activeThreads; i++)
      {
         pthread_join(threads[i], NULL);
      }

      // Clean up
      free(threads);
   }
   else
   {
      serial_architecture();
   }

   printf("Total bad files: %d\n", badFiles);
   printf("Total directories: %d\n", directories);
   printf("Total regular files: %d\n", regularFiles);
   printf("Total special files: %d\n", specialFiles);
   printf("Total bytes used by regular files: %d\n", totalRegularFileBytes);
   printf("Total regular text files: %d\n", textRegularFiles);
   printf("Total bytes used by text files: %d\n", totalTextRegularFileBytes);

   return 0;
}

/* Checks textfile for serial architecture*/
bool checkTextFile(char *filename)
{
   FILE *inputFile = fopen(filename, "r");

   if (inputFile == NULL)
   {
      perror("This file could not be opened - Cannot check if is textfile");
      return false;
   }

   // Iterate through each char and check if its ascii
   int character;
   while ((character = fgetc(inputFile)) != EOF)
   {
      if (isprint(character) == 0 && isspace(character) == 0)
      {
         fclose(inputFile);
         return false;
      }
   }

   fclose(inputFile);
   return true;
}

/* Iterates through files and processes them for serial architecture*/
void serial_architecture()
{
   struct stat filestats;
   char filename[256];

   while (scanf("%s", filename) != EOF)
   {

      if (stat(filename, &filestats) == -1)
      {
         badFiles++;
         continue;
      }

      if (S_ISDIR(filestats.st_mode))
      {
         directories++;
      }

      if (S_ISREG(filestats.st_mode))
      {
         regularFiles++;
         totalRegularFileBytes += (int)filestats.st_size;

         if (checkTextFile(filename) == true)
         {
            textRegularFiles++;
            totalTextRegularFileBytes += (int)filestats.st_size;
         }
      }

      else
      {
         specialFiles++;
      }
   }
}

/* Processes file within each thread*/
void *fileProcessingFunction(void *arg)
{
   char *filename = (char *)arg;

   struct stat filestats;

   if (stat(filename, &filestats) == -1)
   {
      pthread_mutex_lock(&mutex);
      badFiles++;
      pthread_mutex_unlock(&mutex);
      free(filename);
      return NULL;
   }

   if (S_ISDIR(filestats.st_mode))
   {
      pthread_mutex_lock(&mutex);
      directories++;
      pthread_mutex_unlock(&mutex);
   }

   else if (S_ISREG(filestats.st_mode))
   {
      pthread_mutex_lock(&mutex);
      regularFiles++;
      totalRegularFileBytes += (int)filestats.st_size;
      pthread_mutex_unlock(&mutex);

      if (checkTextFile(filename))
      {
         pthread_mutex_lock(&mutex);
         textRegularFiles++;
         totalTextRegularFileBytes += (int)filestats.st_size;
         pthread_mutex_unlock(&mutex);
      }
   }

   else
   {
      pthread_mutex_lock(&mutex);
      specialFiles++;
      pthread_mutex_unlock(&mutex);
   }

   free(filename);
}
