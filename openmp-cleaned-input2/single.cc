#include	<stdio.h>
#include	<stdlib.h>
#include	<omp.h>

int main()
{
	int id, i, a, b[5];
	printf( "Testing the single construct\n" );

	#pragma omp parallel shared(a,b) private(i,id) num_threads(4)
	{
		#pragma omp single
		{
			a = 10;
			id = omp_get_thread_num();
			printf( "Single construct executed by thread %d\n", id );
		}

		#pragma omp for
		for( i = 0; i < 5; i++ )
		{	
			id = omp_get_thread_num();
			printf( "Thread %d is setting b[%d]=%d\n", id, i, a );
			b[i] = a;
		}
	}

	printf( "After the parallel region:\n" );
	printf( "a = %d\n", a );
	for( i = 0; i < 5; i++ )
		printf( "b[%d] = %d\n", i, b[i] );
		
	return(0);
}
