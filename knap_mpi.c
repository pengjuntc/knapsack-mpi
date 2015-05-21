#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <sys/time.h>

void init_things(int n, int max, int weight[n], int profit[n]) 
{
	int i = 0;
	
	for (i = 0; i < n; i++)	{
		weight[i] = (random() % max) + 1;
		profit[i] = (random() % max) + 1;
	}
}

void print_things(int n, int weight[n], int profit[n]) 
{
	int i = 0;
	printf("Weights\n");	
	for (i = 0; i < n; i++)	{
                printf("%2d ", weight[i]);
	}
	printf("\n");

	printf("Profits\n");	
	for (i = 0; i < n; i++)	{
                printf("%2d ", profit[i]);
	}
	printf("\n");
}

void print_table(int n, int c, int total[n][c], int use[n][c])
{
        int i, j;

        printf("Total Profit\n");
        for (j = 0; j < c; j++) {
                for (i = 0; i < n; i++) {
                        printf("%2d ",total[i][j]);
                }
                printf("\n");
        }

        printf("Flags\n");
        for (j = 0; j < c; j++) {
                for (i = 0; i < n; i++) {
                        printf("%2d ",use[i][j]);
                }
                printf("\n");
        }
}


//subblock size
#define ROW 64
#define COL 512

int min(int i, int j) {
        return (i<j) ? i : j;
}


void solver(int n, int c, int rows, int weight[rows], int profit[rows], 
            int start, int rank, int size) {
        int recv_rank = (rank-1)%size;   //rank to receive data
        int send_rank = (rank+1)%size; // rank to send data
        if( start == 0 ) {      // deal with first block, since it doesn't receive data from any nodes
                int total[rows][c];                
                int i, j;
		
                for (j = 0; j < c; j += COL) {
                        int cols = min(COL, c-j);
                        int k;
                        for (k = j; k < j + cols; k++) {
                                if (weight[0] > k) {
                                        total[0][k] = 0;
                                } else {
                                        total[0][k] = profit[0];
                                }        
                        }
			//compute subblock
                        for (i = 1; i < rows; i++) {
                                for (k = j; k < j + cols; k++) {
                                        //int ni = i+start;
                                        if ( (k<weight[i]) || 
                                             (total[i-1][k] >= total[i-1][k-weight[i]] + profit[i])) {
                                                total[i][k] = total[i-1][k];
                                        } else {
                                                total[i][k] = total[i-1][k-weight[i]] + profit[i];
                                        }
                                }       
                        }
			//send last row to next node
                        MPI_Send(&total[rows-1][j], cols, MPI_INT, send_rank, j, MPI_COMM_WORLD);
                }

        } else {
                int total[rows+1][c];  // use the first row to store the data from last node
                int i, j;
                for (j = 0; j < c; j += COL) {
                        int cols = min(COL, c-j);
                        int k;
			// receive data from last node
                        MPI_Recv(&total[0][j], cols, MPI_INT, recv_rank, j, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        for (i = 1; i <= rows; i++) {
                                for (k = j; k < j + cols; k++) {
                                        int ni = i-1;  //ni is index for weight and profit, notice the first row is the data from last node
                                        if ( (k<weight[ni]) || 
                                             (total[i-1][k] >= total[i-1][k-weight[ni]] + profit[ni])) {
                                                total[i][k] = total[i-1][k];
                                        } else {
                                                total[i][k] = total[i-1][k-weight[ni]] + profit[ni];
                                        }
                                }
                        }
			
			
			if (start + rows == n && j + cols == c) {
                                //computer the last subblock of last ROW, print the final result 
                                printf("max profit: %d \n", total[rows][c-1]);
			} else if (start + rows != n){
                                // if it is not last ROW, we need send data to next node.
                                MPI_Send(&total[rows][j], cols, MPI_INT, send_rank, j, MPI_COMM_WORLD);
			}
		}
        }
}



int main(int argc, char *argv[]) {
	int i, n = 0, c = 0, m = 50;
	//	int *weights, *profits;
	unsigned long long usec;
  	struct timeval tstart, tend;
	int (*table)[c], (*flags)[c];

	if (argc > 2 && argc < 5) {
		n = atoi(argv[1]);		/* Number of things */
		c = atoi(argv[2]);		/* Capacity of knapsack */
		if (argc == 4)
			m = atoi(argv[3]);
	}
	if ((n <= 0) || (c <= 0) || (m <= 0)) {
	  	fprintf(stdout, "usage %s n c - where n and c >= 0\n", argv[0]);
		exit(1);
	}
	
	//weights = malloc(sizeof(int [n]));
	//profits = malloc(sizeof(int [n]));
	//	init_things(n, m, weights, profits);
	//print_things(n, weights, profits);
	//table = malloc(sizeof (int [n][c]));
	//flags = malloc(sizeof (int [n][c]));
        int size, rank;
        MPI_Init(NULL, NULL);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	gettimeofday(&tstart, NULL);
	for (i = 0; i < n; i += ROW) {
                int rows  = min(ROW, n-i);
		int *weights = malloc(rows * sizeof(int));   //initial weights locally
		int *profits = malloc(rows * sizeof(int));   //initial weights locally
		init_things(rows, m, weights, profits);
                if ((i/ROW) % size == rank)
                        solver(n, c, rows, weights, profits, i, rank, size);  //solve subblock
		free(weights);
		free(profits);
        }
        //solver(n, c, weights, profits, table, flags);
	gettimeofday(&tend, NULL);

	if (tend.tv_usec > tstart.tv_usec) {
		usec = (tend.tv_sec - tstart.tv_sec) * 1000000
			+ tend.tv_usec - tstart.tv_usec;
	} else {
		usec = (tend.tv_sec - (tstart.tv_sec + 1)) * 1000000
			+ (1000000 + tend.tv_usec - tstart.tv_usec);
	}

        if (rank == 0) {
                fprintf(stdout,
                        "%d * %d in %d nodes Solver finished in %f seconds.\n", n, c, size, (double)usec/1000000.0);
        }

        MPI_Finalize();
	//print_table(n,c,table,flags);
}
