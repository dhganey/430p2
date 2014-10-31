
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
void* func2(void* paramStruct)
{
id = ((StartEnd*) paramStruct)->threadNum;
sum += local_sum;
printf( "Thread %d: local_sum = %d, sum = %d\n", id, local_sum, sum );
}
 a;
int sum;
void* func1(void* paramStruct)
{
int local_sum;
int i;
int id;
local_sum = 0;

#pragma omp fo 
for( i = 0; i < 5; i++ )
local_sum += a[i];

pthread_t threads[-1];
for (int uniqueVar4 = 0; uniqueVar4 < -1; uniqueVar4++)
{
threads[uniqueVar4] = uniqueVar4;
}
for (int uniqueVar5 = 0; uniqueVar5 < -1; uniqueVar5++)
{
StartEnd paramStruct2;
paramStruct2.threadNum = uniqueVar5;
pthread_create(&threads[uniqueVar5], NULL, func2, (void*) &paramStruct2);
}
for (int uniqueVar6 = 0; uniqueVar6 < -1; uniqueVar6++)
{
pthread_join(threads[uniqueVar6], NULL);
}
}
int main()
{

printf( "Example of the critical construct\n" );

for( i = 0; i < 5; i++ )
a[i] = i;

sum = 0;

pthread_t threads[4];
for (int uniqueVar1 = 0; uniqueVar1 < 4; uniqueVar1++)
{
threads[uniqueVar1] = uniqueVar1;
}
for (int uniqueVar2 = 0; uniqueVar2 < 4; uniqueVar2++)
{
StartEnd paramStruct1;
paramStruct1.threadNum = uniqueVar2;
pthread_create(&threads[uniqueVar2], NULL, func1, (void*) &paramStruct1);
}
for (int uniqueVar3 = 0; uniqueVar3 < 4; uniqueVar3++)
{
pthread_join(threads[uniqueVar3], NULL);
}

printf( "Sum should be 5(4)/2 = %d\n", 5*(5-1)/2 );
printf( "Value of sum after parallel region: %d\n", sum );
return(0);
}
