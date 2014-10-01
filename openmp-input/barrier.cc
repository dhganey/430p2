#include	<stdio.h>
#include	<stdlib.h>
#include	<time.h>
#include	<omp.h>

void print_time( int tid, char *s )
{
	time_t timer = time(NULL);

	printf( "Thread %d %s barrier at ", tid, s );
	printf( "%s", ctime( &timer ) );
}

int main()
{
	int tid;

	printf( "Illustrate the barrier construct\n" );

#pragma omp parallel private(tid) num_threads(4)
{
	tid = omp_get_thread_num();
	if( tid < 2 )
		system( "sleep 1" );

	(void) print_time( tid, (char * ) "before" );

	#pragma omp barrier

	(void) print_time( tid, (char * ) "after" );
}
	return(0);
}
