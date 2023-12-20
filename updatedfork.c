#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>

// #include <readline/readline.h>
// #include <readline/history.h>

// Function prototypes
void shell();

int main(int argc, char *argv[])
{

   if (argc < 2)
   {
      shell();
   }

   char *argvNew[5];

   argvNew[0] = argv[1];
   argvNew[1] = argv[2];
   argvNew[2] = NULL;

   // Initialize variables
   int pid;
   struct rusage usage;
   struct timeval time_start;
   struct timeval time_end;

   gettimeofday(&time_start, NULL);

   if ((pid = fork()) < 0)
   {
      perror("Fork error\n");
      exit(1);
   }

   else if (pid == 0)
   {
      execvp(argvNew[0], argvNew);
      perror("This command was not able to run");
      exit(1);
   }

   else
   {
      // Time of wait
      wait(NULL);
      getrusage(RUSAGE_CHILDREN, &usage);
      gettimeofday(&time_end, NULL);

      // CPU time
      long int CPU_time = ((usage.ru_utime.tv_sec) * 1000) + ((usage.ru_utime.tv_usec) / 1000) +
                          ((usage.ru_stime.tv_sec) * 1000) + ((usage.ru_stime.tv_usec) / 1000);

      printf("Total CPU time: %ld milli-seconds\n", CPU_time);

      // Elapsed wall-clock time
      long int elapsed_time = ((time_end.tv_sec - time_start.tv_sec) * 1000) + ((time_end.tv_usec - time_start.tv_usec) / 1000);
      printf("Elapsed “wall-clock” time for the command to execute: %ld \n", elapsed_time);

      // Number of times the process was preempted involuntarily
      long int involuntary = usage.ru_nivcsw;
      printf("Number of times the process was preempted involuntarily: %ld \n", involuntary);

      // Number of times the process gave up the CPU voluntarily
      long int voluntary = usage.ru_nvcsw;
      printf("Number of times the process gave up the CPU voluntarily: %ld \n", voluntary);

      // Number of major page faults, which require disk I/O
      long int hard_faults = usage.ru_majflt;
      printf("Number of major page faults: %ld\n", hard_faults);

      // Number of minor page faults, which could be satisfied without disk I/O
      long int soft_faults = usage.ru_minflt;
      printf("Number of minor page fault: %ld\n", soft_faults);

      // Maximum resident set size used, which is given in kilobytes
      long int resident_set_size = usage.ru_maxrss;
      printf("Maximum resident set size: %ld \n", resident_set_size / 1000);
   }

   return 0;
}

void shell()
{
   // Initialize stats variables for shell
   int pid;
   struct rusage usage;
   struct timeval time_start;
   struct timeval time_end;
   gettimeofday(&time_start, NULL);


   // Initialize input and exit command
   int exit_cmd = 0;
   char input[128];

   // Allocate memory in args_array
   char **args_array;
   args_array = malloc(32 * sizeof(char *));

   // Allocate memory for prompt
   char *prompt;
   prompt = malloc(128);
   strcpy(prompt, "==>");

   // Enter while loop for contiuous prompting
   while (exit_cmd == 0)
   {

      printf("%s", prompt);
      fgets(input, sizeof(input), stdin);

      // Removing the newlinecharacter from input
      input[strcspn(input, "\n")] = '\0';

      // Break apart input
      char *args = strtok(input, " ");
      int n = 0; // Iterator

      while (args != NULL)
      {
         args_array[n++] = args;
         args = strtok(NULL, " ");
      }

      // Finding specific arguments
      if (args_array[0] != NULL)
      {
         if (strcmp(args_array[0], "cd") == 0)
         {
            chdir(args_array[1]);
            continue;
         }

         // Replacing prompt with new prompt
         if ((strcmp(args_array[0], "set") == 0) && (strcmp(args_array[1], "prompt") == 0) && (strcmp(args_array[2], "=") == 0)) // memory dump
         {
            if (args_array[3] != NULL)
            {
               strcpy(prompt, args_array[3]);
            }
         }

         // Exit command
         if (strcmp("exit", input) == 0)
         {
            exit(1);
         }
         else
         {
            // Command execution
            int pid = fork();

            // Child process
            if (pid == 0)
            {
               char *cmd = args_array[0];
               args_array[n] = NULL;
               execvp(cmd, args_array);
               exit(1);
            }
            else
            {
               // Parent process
               // Time of wait
               wait(NULL);
               getrusage(RUSAGE_CHILDREN, &usage);
               gettimeofday(&time_end, NULL);

               // CPU time
               long int CPU_time = ((usage.ru_utime.tv_sec) * 1000) + ((usage.ru_utime.tv_usec) / 1000) +
                                 ((usage.ru_stime.tv_sec) * 1000) + ((usage.ru_stime.tv_usec) / 1000);

               printf("Total CPU time: %ld milli-seconds\n", CPU_time);

               // Elapsed wall-clock time
               long int elapsed_time = ((time_end.tv_sec - time_start.tv_sec) * 1000) + ((time_end.tv_usec - time_start.tv_usec) / 1000);
               printf("Elapsed “wall-clock” time for the command to execute: %ld \n", elapsed_time);

               // Number of times the process was preempted involuntarily
               long int involuntary = usage.ru_nivcsw;
               printf("Number of times the process was preempted involuntarily: %ld \n", involuntary);

               // Number of times the process gave up the CPU voluntarily
               long int voluntary = usage.ru_nvcsw;
               printf("Number of times the process gave up the CPU voluntarily: %ld \n", voluntary);

               // Number of major page faults, which require disk I/O
               long int hard_faults = usage.ru_majflt;
               printf("Number of major page faults: %ld\n", hard_faults);

               // Number of minor page faults, which could be satisfied without disk I/O
               long int soft_faults = usage.ru_minflt;
               printf("Number of minor page fault: %ld\n", soft_faults);

               // Maximum resident set size used, which is given in kilobytes
               long int resident_set_size = usage.ru_maxrss;
               printf("Maximum resident set size: %ld \n", resident_set_size / 1000);
            }
         }
      }
   }
}
