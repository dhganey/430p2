#include	<stdio.h>
#include	<stdlib.h>
#include	<omp.h>

int main()
{
	int i, id;

	#pragma omp parallel for private(i,id) num_threads(4)
	{
		for( i = 0; i < 16; i++ )
		{
			id = omp_get_thread_num();
			printf( "Thread %d executes loop iteration %d\n", id, i );
		}
	}

	return(0);
}
