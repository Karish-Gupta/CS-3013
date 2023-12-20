#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXGRID 40

// Global variables
int ROWS;
int COLS;
int board[MAXGRID][MAXGRID];
int newboard[MAXGRID][MAXGRID];

// Print game board
void printBoard()
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            if (board[i][j] == 1)
            {
                printf("1 "); // Alive cell
            }
            else
            {
                printf("0 "); // Dead cell
            }
        }
        printf("\n");
    }
    printf("\n");
}

// Update board
void updateBoard()
{
    int newBoard[ROWS][COLS];

    for (int i = 0; i < ROWS; i++)
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
                    newBoard[i][j] = 0; // Death
                }
                else
                {
                    newBoard[i][j] = 1; // Survive
                }
            }
            else
            {
                if (neighbors == 3)
                {
                    newBoard[i][j] = 1; // Born
                }
                else
                {
                    newBoard[i][j] = 0; // Dead cell
                }
            }
        }
    }

    // Replace board with new generation
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            board[i][j] = newBoard[i][j];
        }
    }
}

void readGameboard(char *filename)
{
    //  Initialize variables
    int file_descriptor;
    struct stat file_stats;
    int read_bytes;

    FILE *boardFile = fopen(filename, "r");
    if (filename == NULL)
    {
        perror("Error opening the input file");
        exit(1);
    }

    // Open file
    file_descriptor = open(filename, O_RDONLY);
    if (file_descriptor < 0)
    {
        perror("Failed to open file");
        exit(1);
    }

    // Get file size
    if (fstat(file_descriptor, &file_stats) == -1)
    {
        perror("File statistics failed");
        exit(1);
    }

    char buffer[4000];
    read_bytes = read(file_descriptor, buffer, 6400);

    char line[80];
    int numRows = 0;
    int numCols = 0;

    while (fgets(line, sizeof(line), boardFile) != NULL)
    {
        // Skip empty lines
        if (line[0] == '\n')
        {
            continue;
        }

        // First count col in first line
        if (numCols == 0)
        {
            char *token = strtok(line, " ");
            while (token != NULL)
            {
                token = strtok(NULL, " ");
                (numCols)++;
            }
        }

        (numRows)++;
    }
    ROWS = numRows;
    COLS = numCols;

    fclose(boardFile);

    // Initialize the gen 0 board

    int k = 0;
    for (int i = 0; i < numRows; i++)
    {
        for (int j = 0; j < numCols; j++)
        {
            board[i][j] = (buffer[k] - 48);
            k += 2;
        }
    }
}
