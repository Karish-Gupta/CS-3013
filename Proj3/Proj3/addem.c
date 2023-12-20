#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define MAXTHREAD 10
#define RANGE 1
#define ALLDONE 2

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
void *myThread(void *arg);
struct msg create_message(int iSender, int type, int value1, int value2);

// Global variables
struct msg *mailboxes;
sem_t semaphores_producer[MAXTHREAD + 1];
sem_t semaphores_consumer[MAXTHREAD + 1];
int thread_indices[MAXTHREAD + 1];

int total = 0;

/*Main thread*/
int main(int argc, char *argv[])
{

   if (argc < 3)
   {
      perror("please enter a number of threads followed by a number to sum up to");
      return 1;
   }

   int num_threads = atoi(argv[1]);
   int sum_limit = atoi(argv[2]);

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
      pthread_create(&thread_ids[i], NULL, myThread, (void *)(i + 1));
   }

   int sum_range = sum_limit / num_threads; 
   int remainder = sum_limit % num_threads; 

   int start = 1;

   for (int i = 0; i < num_threads; i++)
   {
      int start_range = start;
      int end_range = start + sum_range - 1;

      // Distribute remainder 
      if (i < remainder)
      {
         end_range++;
      }

      struct msg sum_message = create_message(0, RANGE, start_range, end_range);
      SendMsg(i + 1, &sum_message);

      start = end_range + 1;
   }

   for (int i = 0; i < num_threads; i++)
   {
      struct msg all_done_message;
      RecvMsg(0, &all_done_message);
      total += all_done_message.value1;
   }

   printf("The total for 1 to %d using %d threads is %d.\n", sum_limit, num_threads, total);

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
void *myThread(void *arg)
{
   int thread_index = (int)arg;

   struct msg sum_message;
   RecvMsg(thread_index, &sum_message);

   // Calculate sum
   int sum = 0;
   for (int i = sum_message.value1; i <= sum_message.value2; i++)
   {
      sum += i;
   }

   // Send alldone message to the parent thread
   struct msg alldone_message = create_message(thread_index, ALLDONE, sum, 0);
   SendMsg(0, &alldone_message);

   return NULL;
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