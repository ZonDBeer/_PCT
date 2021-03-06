#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/time.h>

using namespace std;

#define NUM_THREADS 4
#define N 100000000 // number of elements in array X
 
int *X = new int [N];
int gSum[NUM_THREADS]; // global storage for partial results

/* getrand: Returns random number from [min, max) */
int getrand(int min, int max)
{
    return (double)rand() / (RAND_MAX + 1.0) * (max - min) + min;
}

/* wtime: Returns time */
double wtime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec * 1E-6;
}

void InitializeArray(int *X)
{
	int i = 0;
	for (i = 0; i < N; ++i) {
		X[i] = getrand(-10, 10);
	}
}

void *Summation (void *pArg)
{
	int tNum = *((int *) pArg);
	int lSum = 0;
	int start, end;

	start = (N/NUM_THREADS) * tNum;
	end = (N/NUM_THREADS) * (tNum+1);
	
	if (tNum == (NUM_THREADS-1)) end = N;
	
	for (int i = start; i < end; i++)
		lSum += X[i];
		
	gSum[tNum] = lSum;
	free(pArg);
	
	return 0;
}

int main(int argc, char* argv[])
{
	int j;
	double t, t2;
	long long int sum = 0, sum2 = 0;
	pthread_t tHandles[NUM_THREADS];
	
	InitializeArray(X); // get values into X array; not shown
	
	t = wtime();	
	for (j = 0; j < NUM_THREADS; j++) {
		int *threadNum = new(int);
		*threadNum = j;
		pthread_create(&tHandles[j], NULL, Summation, (void *)threadNum);
	}
	
	for (j = 0; j < NUM_THREADS; j++) {
		pthread_join(tHandles[j], NULL);
		sum += gSum[j];				
	}
	t = wtime() - t;
	
	t2 = wtime();
	for (int i = 0; i < N; ++i)
		sum2 += X[i];
	t2 = wtime() - t2;
	
	cout << "The sum of array elements is " << sum << endl;
	cout << "Time1: " << t << endl;
	cout << "Time2: " << t2 << endl;
	return 0;
}
