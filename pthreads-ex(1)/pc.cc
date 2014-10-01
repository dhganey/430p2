// Implements the classic producer consumer problem.

#include	<pthread.h>
#include 	<stdio.h>
#include 	<stdlib.h>
#include	<unistd.h>
#include 	<semaphore.h>

#define BUFFER_SIZE 	5	/* total number of slots */

int seed;

typedef struct {
    int buf[BUFFER_SIZE]; /* shared var */
    int in;         	  /* buf[in%BUFFER_SIZE] is the first empty slot */
    int out;        	  /* buf[out%BUFFER_SIZE] is the first full slot */
    sem_t full;     	  /* keep track of the number of full spots */
    sem_t empty;    	  /* keep track of the number of empty spots */
    sem_t mutex;    	  /* enforce mutual exclusion to shared data */
} sbuf_t;

sbuf_t shared;

void *
Producer(void *arg)
{
    int i, item;

    /* Rather than have the producer loop forever, it loops a finite number of times. */

    for( i = 0; i < 10; i++ )
    {
        /* Produce item */

        item = i;	

        /* Wait for an empty slot. */

        sem_wait( &shared.empty );

        /* Insert the item into the buffer inside a critical section. */

        sem_wait( &shared.mutex );
            shared.buf[ shared.in ] = item;
            printf("[P] producing item %d in buffer[%d]...\n", item, shared.in ); 
	    fflush(stdout);
            shared.in = (shared.in+1)%BUFFER_SIZE;
        sem_post( &shared.mutex );

        /* Signal the addition of another full slot */

        sem_post( &shared.full );
	sleep( rand()%3 );
    }

    printf( "Producer thread terminating...\n" );
    fflush( stdout );
    pthread_exit( NULL );
}

void *
Consumer(void *arg)
{
    int i, item;

    /* The consumer loops forever and will be killed by the main thread terminating. */

    while( true )
    {
	/* Wait for a slot in the buffer to be full. */

        sem_wait( &shared.full );

	/* Remove the item from the buffer in a critical section. */

        sem_wait( &shared.mutex );
            item = shared.buf[ shared.out ];
	    printf( "[C] consuming item %d from buffer[%d] ...\n", item, shared.out );
	    fflush( stdout );
            shared.out = (shared.out+1)%BUFFER_SIZE;
        sem_post( &shared.mutex );

	/* Signal that a slot has been emptied. */

	sem_post( &shared.empty );
    }
}

int main()
{
    pthread_t idP, idC;

    printf( "Give an integer seed for used to randomize interleaving of producer/consumer\n" );
    scanf( "%d", &seed );
    srand( seed );

    /* Initialize the full semaphore to zero because the number of full slots is initially zero. */
    /* Initialize the empty sempahore to the buffer size because the entire buffer is empty. */
    /* Initialize the mutex semaphore to one because it is used for mutual exclusion. */

    sem_init( &shared.full, 0, 0 );
    sem_init( &shared.empty, 0, BUFFER_SIZE );
    sem_init( &shared.mutex, 0, 1 );

    /* Create a new producer */

    if( pthread_create( &idP, NULL, Producer, NULL ) != 0 )
    {
	printf( "Error: unable to create producer thread\n" );
	exit( -1 );
    }

    /* Create a new consumer */
    if( pthread_create( &idC, NULL, Consumer, NULL ) != 0 )
    {
	printf( "Error: unable to create consumer thread\n" );
	exit( -1 );
    }

    pthread_join( idP, NULL );
    if( pthread_cancel( idC ) == 0 )
    {
        printf( "Consumer thread cancelled...\n" );
        fflush( stdout );
    }
    printf( "main() terminating\n" );
    pthread_exit( NULL );
}
