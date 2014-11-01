#include	<stdio.h>
#include	<stdlib.h>
#include	<omp.h>
#include <pthread.h>
#include <algorithm>
#include <cstring>

struct StartEnd
{
	int start;
	int end;
	int threadNum;
};

int a;
int b[5];

void* func2(void* paramStruct)
{
	int i;
	int id;
	for (  i  = ((StartEnd*)paramStruct)->start;  i  < ((StartEnd*)paramStruct)->end;  i ++)
	{	
		id = ((StartEnd*) paramStruct)->threadNum;
		printf( "Thread %d is setting b[%d]=%d\n", id, i, a );
		b[i] = a;
	}
}

void* func1(void* paramStruct)
{
	int i;
	int id;
	if (((StartEnd*) paramStruct)->threadNum == 0) //arbitrarily restrict it to the only guaranteed thread, 0
	{
		a = 10;
		id = ((StartEnd*) paramStruct)->threadNum;
		printf( "Single construct executed by thread %d\n", id );
	}

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
}

int main()
{
	int id, i, a, b[5];
	printf( "Testing the single construct\n" );

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

	printf( "After the parallel region:\n" );
	printf( "a = %d\n", a );
	for( i = 0; i < 5; i++ )
	printf( "b[%d] = %d\n", i, b[i] );

	return(0);
}
