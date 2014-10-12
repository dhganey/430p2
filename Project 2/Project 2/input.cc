#include	<stdio.h>
#include	<stdlib.h>
#include	<omp.h>

int main()
{
	int i, n;

#pragma omp parallel for num_threads(4)
	for( i = 0; i < 16; i++ )
		printf( "Thread %d executes loop iteration %d\n", omp_get_thread_num(), i );

	return(0);
}
