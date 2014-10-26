#include	<stdio.h>
#include	<stdlib.h>
#include	<omp.h>

int main()
{
	int id, i, a[5], local_sum, sum;
	printf( "Example of the critical construct\n" );

	for( i = 0; i < 5; i++ )
		a[i] = i;

	sum = 0;

	#pragma omp parallel shared(a,sum) private(local_sum,i,id) num_threads(4)
	{
		local_sum = 0;
	
		#pragma omp for 
		{
			for( i = 0; i < 5; i++ )
				local_sum += a[i];
		}

		#pragma omp critical
		{
			id = omp_get_thread_num();
			sum += local_sum;
			printf( "Thread %d: local_sum = %d, sum = %d\n", id, local_sum, sum );
		}
	}

	printf( "Sum should be 5(4)/2 = %d\n", 5*(5-1)/2 );
	printf( "Value of sum after parallel region: %d\n", sum );
	return(0);
}
