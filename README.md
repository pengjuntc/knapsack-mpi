The folder contains four files. 
(1) knap.c, which is the sequential code for knapsack problem, provided by the professor.
(2) knap_mpi.c, which is parallel version in MPI for knapsack problem.
(3) makefile, which compiles knap.c and knap_mpi.c, generating executable knap and mpi respectively.
(4) README.txt, which is this file, explaining the parallel code and describing how to test the code.
(5) run.pbs, which is the script file for galles machine to run.
(6) screenshot.png, which proves my test result.

**********explain the parallel code*********
Basically, total[i][j] records the maximum value for i items (from 1 to i) with maximum capacity j.
Recurrence Relation:
  (index i -- ith item; index j -- capacity j)
  1. total[i][j] = total[i-1][j] if weight[i] > j or total[i-1][j] >= total[i-1][j-weight[i]] + profit[i]
  2. total[i][j] = total[i-1][j-weight[i]] + profit[i]    otherwise

Since the total table is a 2D array, we can partition the array by rows. Defining constant #ROW in knap_mpi.c, we cyclicly assign #ROW rows to nodes shown in below. Every node needs compute a 2D array with dimension #ROW * capacity. In the reccurence relation we can see that node 1 needs the last row of nodes 0 to do the calculation (like total[i][j] = total[i-1][j]). In other words, when we calculate the total[i][j], we need to know the data above total[i-1][j] from colum 0 to column j. In order to fully exploit parallelism, we partition #ROW * capacity into several subblocks with #ROW * COL. We compute the subblock sequentially. When we finish one subblock, we send the last row of the subblock to next node and compute next subblock. The next node receives the row from its previous node and can start compute its subblock. When it's done, it sends the last row to its next node and wait to receive the data from its previous node for next subblock. In this way, we compute the 2D array in a diagonal way to maximize the parallelism.  




                 		capacity   
     		COL
	-----------------------------------------------------------------
	|R              	|                      	|		|
    N	|O              	|                     	|               |  rank 0        
    U	|W     last row		|		      	|		|
    M	-----------------------------------------------------------------
    B	|wait data from last row|		      	|		|
    E	|			|		      	|		| 	1
    R	|send data to next row	|			|		|
	-----------------------------------------------------------------
    OF	|						|		|
	|						|		|	2
  Items	|						|		|
	-----------------------------------------------------------------
	|						|		|



**********general test instruction**********
1. log into galles.alliance.unm.edu and upload codes to galles.
2. run command: make
   it will generate two executables knap and mpi
2. test sequential version: ./knap n c
   n is the number of items, c is the maximum capcity
3. test parallel version: qsub run.pbs
   it will turn in the task to galles machine and execute the script.
4. check run.pbs.oXXXX to see the result



****************test result*****************
The program is tested on freightliner, which its CPU has 8 processors. 
1. strong scaling
   For strong scaling, we test the squential version for 10000*10000 and MPI version for 10000*10000 with 16 nodes. 
   The time for squential version is 0.616506 s.
   The time for MPI version is 0.142661 s.
   The efficiency = 0.616506/(0.142661*16) = 28.7%

2. weak scaling
   For weak scaling, we test the MPI version for 20000*20000 with 8 nodes and 20000*40000 with 16 nodes.
   The time with 8 nodes is 1.003893 s.
   The time with 16 nodes is 1.041984 s.
   The isoefficiency = 1.003893/1.041984 = 96.3%
   
