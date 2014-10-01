#include	<stdio.h>
#include	<stdlib.h>
#include	<omp.h>

int main()
{
	int i, a, b[5];
	printf( "Testing the single construct\n" );

#pragma omp parallel shared(a,b) private(i) num_threads(4)
{
	#pragma omp single
	{
		a = 10;
		printf( "Single construct executed by thread %d\n", omp_get_thread_num() );
	}

	#pragma omp for
	for( i = 0; i < 5; i++ )
	{	
		printf( "Thread %d is setting b[%d]=%d\n", omp_get_thread_num(), i, a );
		b[i] = a;
	}
}

	printf( "After the parallel region:\n" );
	printf( "a = %d\n", a );
	for( i = 0; i < 5; i++ )
		printf( "b[%d] = %d\n", i, b[i] );
		
	return(0);
}
