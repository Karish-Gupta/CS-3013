#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "life_functions.h"

#define RANGE 1
#define ALLDONE 2
#define GO 3
#define GENDONE 4 // Generation Done
#define MAXTHREAD 10
#define MAXGRID 40

struct msg
{
   int iSender; /* sender of the message (0 .. number-of-threads) */
   int type;    /* its type */
   int value1;  /* first value */
   int value2;  /* second value */
};

// Function prototypes
int allocate_mailboxes();
void SendMsg(int iTo, struct msg *pMsg);
void RecvMsg(int iFrom, struct msg *pMsg);
void *worker_thread(void *arg);
struct msg create_message(int iSender, int type, int value1, int value2);
void updateRows(int start_row, int end_row);
void send_range();

// Global variables
struct msg *mailboxes;
sem_t semaphores_producer[MAXTHREAD + 1];
sem_t semaphores_consumer[MAXTHREAD + 1];
int thread_indices[MAXTHREAD + 1];
int num_threads;
int numGen;

int total = 0;

/*Main thread*/
int main(int argc, char *argv[])
{

   // Get arguments
   if (argc < 2)
   {
      perror("Please enter number of threads, filename, and number of generations");
   }

   num_threads = atoi(argv[1]);

   char *filename = argv[2];

   char *generation = argv[3];
   numGen = atoi(generation);

   char *print = argv[4];
   if (print == NULL)
   {
      print = "n";
   }

   char *input = argv[5];
   if (input == NULL)
   {
      input = "n";
   }
   int allow_print;

   // Initialize board
   readGameboard(filename);

   // Check number of rows
   if (ROWS > MAXGRID || COLS > MAXGRID)
   {
      perror("Rows or columns to large");
      exit(1);
   }

   if (strcmp(print, "y") == 0)
   {
      printf("Generation 0:\n");
      printBoard();
   }

   allocate_mailboxes();

   // Initialize semaphores
   for (int i = 0; i <= MAXTHREAD; i++)
   {
      sem_init(&semaphores_producer[i], 0, 1);
   }
   for (int i = 0; i <= MAXTHREAD; i++)
   {
      sem_init(&semaphores_consumer[i], 0, 0);
   }

   // Create threads
   pthread_t thread_ids[MAXTHREAD];
   for (int i = 0; i < num_threads; i++)
   {
      pthread_create(&thread_ids[i], NULL, worker_thread, (void *)(i + 1));
   }

   send_range(); // Function sends row range to each worker thread

   for (int gen = 1; gen <= numGen; gen++)
   {

      // Send go msg to all thrads
      for (int i = 0; i < num_threads; i++)
      {
         struct msg go_msg;
         SendMsg(i + 1, &go_msg);
      }

      for (int i = 0; i < num_threads; i++)
      {
         struct msg gen_done;
         RecvMsg(0, &gen_done);
      }

      for (int i = 0; i < MAXGRID; i++)
      {
         for (int j = 0; j < MAXGRID; j++)
         {
            board[i][j] = newboard[i][j];
         }
      }
      if (strcmp(print, "y") == 0 || numGen == gen)
      {
         if (strcmp(input, "y") == 0)
         {
            getchar();
            printf("Generation %d:\n", gen);
            printBoard();
            sleep(1); // Sleep for 1 second between generations
         }
         else
         {
            printf("Generation %d:\n", gen);
            printBoard();
            sleep(1); // Sleep for 1 second between generations
         }
      }
   }

   for (int i = 0; i < num_threads; i++)
   {
      struct msg all_done_message;
      RecvMsg(0, &all_done_message);
   }

   // Clean up
   for (int i = 0; i < num_threads; i++)
   {
      pthread_join(thread_ids[i], NULL);
      sem_destroy(&semaphores_producer[i]);
      sem_destroy(&semaphores_consumer[i]);
   }

   free(mailboxes);

   return 0;
}

/* Send a messagge to a mailbox*/
void SendMsg(int iTo, struct msg *pMsg)
{
   sem_wait(&semaphores_producer[iTo]);
   mailboxes[iTo] = *pMsg;
   sem_post(&semaphores_consumer[iTo]);
}

/* Recieve a message from another thread*/
void RecvMsg(int iFrom, struct msg *pMsg)
{
   sem_wait(&semaphores_consumer[iFrom]);
   *pMsg = mailboxes[iFrom];
   sem_post(&semaphores_producer[iFrom]);
}

/* Thread function*/
void *worker_thread(void *arg)
{
   int thread_index = (int)arg;

   struct msg range_msg;
   RecvMsg(thread_index, &range_msg);

   for (int i = 0; i < numGen; i++)
   {

      struct msg go_msg;
      RecvMsg(thread_index, &go_msg);

      updateRows(range_msg.value1, range_msg.value2);

      struct msg gendone_msg = create_message(thread_index, GENDONE, 0, 0);
      SendMsg(0, &gendone_msg);
   }

   // Send alldone message to the parent thread
   struct msg all_done_msg = create_message(thread_index, ALLDONE, 0, 0);
   SendMsg(0, &all_done_msg);
}

/* Allocates memory for threadmailboxes*/
int allocate_mailboxes()
{
   mailboxes = (struct msg *)malloc((MAXTHREAD + 1) * sizeof(struct msg));

   if (mailboxes == NULL)
   {
      printf("Invalid number of threads");
      return 1;
   }

   return 0;
}

/* Creates message struct*/
struct msg create_message(int iSender, int type, int value1, int value2)
{
   struct msg message;
   message.iSender = iSender;
   message.type = type;
   message.value1 = value1;
   message.value2 = value2;

   return message;
}

// Update board
void updateRows(int start_row, int end_row)
{
   for (int i = start_row; i <= end_row; i++)
   {
      for (int j = 0; j < COLS; j++)
      {
         int neighbors = 0;

         // Check the eight neighbors
         for (int x = -1; x <= 1; x++)
         {
            for (int y = -1; y <= 1; y++)
            {
               if (x == 0 && y == 0)
                  continue;
               int neighbor_i = i + x;
               int neighbor_j = j + y;

               if (neighbor_i >= 0 && neighbor_i < ROWS && neighbor_j >= 0 && neighbor_j < COLS)
               {
                  neighbors += board[neighbor_i][neighbor_j];
               }
            }
         }

         // Cell cylce
         if (board[i][j] == 1)
         {
            if (neighbors < 2 || neighbors > 3)
            {
               newboard[i][j] = 0; // Death
            }
            else
            {
               newboard[i][j] = 1; // Survive
            }
         }
         else
         {
            if (neighbors == 3)
            {
               newboard[i][j] = 1; // Born
            }
            else
            {
               newboard[i][j] = 0; // Dead cell
            }
         }
      }
   }
}

void send_range()
{
   // Divide the rows up evenly among threads
   int sum_range = ROWS / num_threads;
   int extra_rows = ROWS % num_threads;
   int start = 0;

   for (int i = 0; i < num_threads; i++)
   {
      int start_range = start;
      int end_range = start + sum_range - 1;

      if (extra_rows > 0)
      {
         // If extra row add it to last thread
         end_range++;
         extra_rows--;
      }

      struct msg range_msg = create_message(0, RANGE, start_range, end_range);
      SendMsg(i + 1, &range_msg);

      start = end_range + 1;
   }
}