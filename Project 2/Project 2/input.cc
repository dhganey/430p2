#include	<stdio.h>
#include	<stdlib.h>
#include	<omp.h>

int main()
{
	int id;
	int sharedTest;

	#pragma omp parallel private(id) shared(sharedTest) num_threads(16)
	{
		id = omp_get_thread_num();

		printf( "The parallel region is executed by thread %d\n", id );

		if( id == 2 )
			printf( "   Thread %d does things differently\n", id );
	}

	return(0);
}
