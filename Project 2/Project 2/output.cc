#include	<stdio.h>
#include	<stdlib.h>
#include	<omp.h>

#include <pthread.h>
#include <algorithm>
#include <cstring>

pthread_mutex_t theMutex = PTHREAD_MUTEX_INITIALIZER;


struct StartEnd
{
	int start;
	int end;
	int threadNum;
};

int a[5];
int sum;

void* func2(void* paramStruct)
{
	int local_sum;
	int i;
	int id;
	for (  i  = ((StartEnd*)paramStruct)->start;  i  < ((StartEnd*)paramStruct)->end;  i ++)
	{
		local_sum += a[i];
	}
}
void* func1(void* paramStruct)
{
	int local_sum;
	int i;
	int id;
	local_sum = 0;

	pthread_t threads[4];
	for (int uniqueVar3 = 0; uniqueVar3 < 4; uniqueVar3++)
	{
		threads[uniqueVar3] = uniqueVar3;
	}

	StartEnd paramStruct0;
	paramStruct0.start = 0;
	paramStruct0.end = 1;
	paramStruct0.threadNum = 0;
	pthread_create(&threads[0], NULL, func2, (void*) &paramStruct0);
	StartEnd paramStruct1;
	paramStruct1.start = 1;
	paramStruct1.end = 2;
	paramStruct1.threadNum = 1;
	pthread_create(&threads[1], NULL, func2, (void*) &paramStruct1);
	StartEnd paramStruct2;
	paramStruct2.start = 2;
	paramStruct2.end = 3;
	paramStruct2.threadNum = 2;
	pthread_create(&threads[2], NULL, func2, (void*) &paramStruct2);
	StartEnd paramStruct3;
	paramStruct3.start = 3;
	paramStruct3.end = 5;
	paramStruct3.threadNum = 3;
	pthread_create(&threads[3], NULL, func2, (void*) &paramStruct3);
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);
	pthread_join(threads[3], NULL);

	pthread_mutex_lock( &theMutex );
	{
		id = ((StartEnd*) paramStruct)->threadNum;
		sum += local_sum;
		printf( "Thread %d: local_sum = %d, sum = %d\n", id, local_sum, sum );
	}
	pthread_mutex_unlock (&theMutex );
}

int main()
{
	int i;
	int id;
	int local_sum;

	printf( "Example of the critical construct\n" );

	for( i = 0; i < 5; i++ )
		a[i] = i;

	sum = 0;

	pthread_t threads[4];

	for (int uniqueVar1 = 0; uniqueVar1 < 4; uniqueVar1++)
	{
		threads[uniqueVar1] = uniqueVar1;
	}

	StartEnd paramStruct0;
	paramStruct0.threadNum = 0;
	pthread_create(&threads[0], NULL, func1, (void*) &paramStruct0);
	StartEnd paramStruct1;
	paramStruct1.threadNum = 1;
	pthread_create(&threads[1], NULL, func1, (void*) &paramStruct1);
	StartEnd paramStruct2;
	paramStruct2.threadNum = 2;
	pthread_create(&threads[2], NULL, func1, (void*) &paramStruct2);
	StartEnd paramStruct3;
	paramStruct3.threadNum = 3;
	pthread_create(&threads[3], NULL, func1, (void*) &paramStruct3);
	for (int uniqueVar2 = 0; uniqueVar2 < 4; uniqueVar2++)
	{
		pthread_join(threads[uniqueVar2], NULL);
	}

	printf( "Sum should be 5(4)/2 = %d\n", 5*(5-1)/2 );
	printf( "Value of sum after parallel region: %d\n", sum );
	return(0);
}
