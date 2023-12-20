#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ctype.h>

// #define DEBUG

// Fuction prototypes
int use_map(char *argv);
int use_read(int chunk, char *filename);
int parallelize(int num_proc, char *filename);
int processChunkChild(char *file_contents, size_t chunk_start, size_t chunk_end);

int main(int argc, char *argv[])
{

   if (argc < 2)
   {
      perror("File not found");
      return 1;
   }

   if (argc >= 3)
   {
      char *filename = argv[1];
      char *mode = argv[2];

      if (strcmp(mode, "mmap") == 0)
      {
         int map = use_map(filename);
         if (map != 0)
         {
            perror("Map failed");
         }
         else
            return 0;
      }

      int chunk = atoi(mode);
      if (chunk > 0 && chunk < 8192)
      {
         int read = use_read(chunk, filename);
         if (read != 0)
         {
            perror("Read failed");
         }
         else
            return 0;
      }

      // Check for p(int)
      char *p_int = &mode[1];
      if ('p' == mode[0])
      {
         int num_proc = atoi(p_int);
         if (num_proc > 1 && num_proc < 17)
         {
            parallelize(num_proc, filename);
            return 0;
         }
         else
         {
            perror("Invalid number of processes");
         }
      }

      else
      {
         use_read(1024, filename);
         return 0;
      }
   }
   else
   {
      use_read(1024, argv[1]);
   }
   return 0;
}

/*
Uses read to process chunks of given file in memory.
*/
int use_read(int chunk, char *filename)
{
   char *buffer = malloc(chunk * sizeof(char));
   int read_bytes;
   int file_descriptor;

   // Initialize character counts
   int ascii_count = 0;
   int uppercase_count = 0;
   int lowercase_count = 0;
   int digit_count = 0;
   int space_count = 0;

   file_descriptor = open(filename, O_RDONLY);
   if (file_descriptor < 0)
   {
      perror("Failed to open file");
      return 1;
   }

   while (read_bytes = read(file_descriptor, buffer, chunk))
   {
      for (int i = 0; i < read_bytes; i++)
      {
         char temp = buffer[i];

         if (isascii(temp) != 0)
         {
            ascii_count++;

            if (isupper(temp) != 0)
            {
               uppercase_count++;
            }

            if (islower(temp) != 0)
            {
               lowercase_count++;
            }

            if (isdigit(temp) != 0)
            {
               digit_count++;
            }

            if (isspace(temp) != 0)
            {
               space_count++;
            }
         }
      }
   }

   printf("ascii=%d, ", ascii_count);
   printf("upper=%d, ", uppercase_count);
   printf("lower=%d, ", lowercase_count);
   printf("digit=%d, ", digit_count);
   printf("space=%d out of %ld bytes \n", space_count, ascii_count);
   close(file_descriptor);
   return 0;
}

/*
Uses mmap to take all data in file and process it in memory
*/
int use_map(char *filename)
{
   // Initialize character counts
   int ascii_count = 0;
   int uppercase_count = 0;
   int lowercase_count = 0;
   int digit_count = 0;
   int space_count = 0;

   //  Initialize variables
   char *file = filename;
   int file_descriptor;
   struct stat file_stats;
   char *content;

   // Open file
   file_descriptor = open(file, O_RDONLY);
   if (file_descriptor < 0)
   {
      perror("Failed to open file");
      return 1;
   }

   // Get file size
   if (fstat(file_descriptor, &file_stats) == -1)
   {
      perror("File statistics failed");
      return 1;
   }

   // Map file into memory
   content = mmap(NULL, file_stats.st_size, PROT_READ, MAP_SHARED, file_descriptor, 0);
   if (content == MAP_FAILED)
   {
      perror("file failed to be mapped to memory");
      close(file_descriptor);
      return 1;
   }

   // Process content
   for (int i = 0; i < file_stats.st_size; i++)
   {
      char temp = content[i];
      if (isascii(temp) != 0)
      {
         ascii_count++;

         if (isupper(temp) != 0)
         {
            uppercase_count++;
         }

         if (islower(temp) != 0)
         {
            lowercase_count++;
         }

         if (isdigit(temp) != 0)
         {
            digit_count++;
         }

         if (isspace(temp) != 0)
         {
            space_count++;
         }
      }
   }

   printf("ascii=%d, ", ascii_count);
   printf("upper=%d, ", uppercase_count);
   printf("lower=%d, ", lowercase_count);
   printf("digit=%d, ", digit_count);
   printf("space=%d out of %ld bytes \n", space_count, file_stats.st_size);
   munmap(content, file_stats.st_size);
   close(file_descriptor);
   return 0;
}

int parallelize(int num_proc, char *filename)
{
#ifdef DEBUG
   printf("Inside parallelization with %d threads...\n", num_proc);
#endif

   //  Initialize variables
   int file_descriptor;
   struct stat file_stats;
   int start_chunk = 0;
   int end_chunk = 0;
   int pid;
   char *content;

   // Open file
   file_descriptor = open(filename, O_RDONLY);
   if (file_descriptor < 0)
   {
      perror("Failed to open file");
      return 1;
   }

   // Get file size
   if (fstat(file_descriptor, &file_stats) == -1)
   {
      perror("File statistics failed");
      return 1;
   }

   // Map to memory
   content = mmap(NULL, file_stats.st_size, PROT_READ, MAP_SHARED, file_descriptor, 0);
   if (content == MAP_FAILED)
   {
      perror("Failed to map file into memory");
      close(file_descriptor);
      return 1;
   }

   // Loop through number of child processes
   for (int i = 0; i < num_proc; i++)
   {

      start_chunk = end_chunk;
      end_chunk = (file_stats.st_size * (i + 1)) / num_proc;

      pid = fork();

      if (pid < 0)
      {
         perror("Child process failed");
         close(file_descriptor);
         return 1;
      }

      if (pid == 0)
      {
         int result = processChunkChild(content, start_chunk, end_chunk);
         exit(0);
      }
   }

   int wpid;
   while ((wpid = wait(0)) > 0)
   {
   }

   return 0;
}

int processChunkChild(char *file_contents, size_t chunk_start, size_t chunk_end)
{

   // Initialize character counts
   int ascii_count = 0;
   int uppercase_count = 0;
   int lowercase_count = 0;
   int digit_count = 0;
   int space_count = 0;

   // Character count variable for this process
   int count = 0;

   // Process the chunk
   for (size_t i = chunk_start; i < chunk_end; i++)
   {
      char temp = file_contents[i];
      if (isascii(temp) != 0)
      {
         ascii_count++;

         if (isupper(temp) != 0)
         {
            uppercase_count++;
         }

         if (islower(temp) != 0)
         {
            lowercase_count++;
         }

         if (isdigit(temp) != 0)
         {
            digit_count++;
         }

         if (isspace(temp) != 0)
         {
            space_count++;
         }
      }
   }

   printf("ascii=%d, ", ascii_count);
   printf("upper=%d, ", uppercase_count);
   printf("lower=%d, ", lowercase_count);
   printf("digit=%d, ", digit_count);
   printf("space=%d out of %ld bytes \n", space_count, ascii_count);

   return count;
}
