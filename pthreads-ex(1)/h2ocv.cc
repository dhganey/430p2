// ****************************************************************************
// This is a sample multi-threaded program to illustrate the use of mutexes
// and condition variables in pthreads.  It uses the same "water making" 
// idea as before where a water molecule is made up of 2 H atoms and one 0 
// atom.  In this program, we spawn off a bunch of H and O atoms.  The atom 
// sleeps a random amount of time.  When it wakes up, if there is enough of 
// the other atoms to make water, water is made.  This is evaluated by looking 
// at the values of some global state variables.  Then the molecule 
// specifically signals two H and one O atoms, which are each waiting to
// on a condition.
// ****************************************************************************

#include	<stdlib.h>
#include	<stdio.h>
#include	<pthread.h>
#include	<semaphore.h>
#include	<unistd.h>

// These global variables will be used as the conditions on which an atom
// will wait.  We will allocate an array for all the O atoms and all the H
// atoms.  If h[i]/o[i] = 0 then the atom has been used in making a water
// molecule, otherwise if h[i]/o[i] = 1 then the atom has not yet been used.
// So the molecule once created will set h[i]/o[i] to 1 as appropriate,
// and then wait until it's been set back to zero, i.e., used.

int *h;
int *o;

int total = 0; // The total number of atoms to create (will be a multiple of 3)

// The mutex (i.e., binary semaphore used to protect access to the state 
// variables below.  Note that a statically allocated mutex (such as this)
// does not need to be explicitly initialized.  By default it is zero.

pthread_mutex_t lock_it;

// Two global state variables:

int h_atoms; // The number of H atoms
int o_atoms; // The number of O atoms

// A mutex (mutex) and condition variable (cv) for use with pthread_cond_wait 
// and pthread_cond_signal.  The _o, _h, indicates whether it's associated
// with O or H atoms, respectively.

pthread_mutex_t mutex_o;
pthread_cond_t cv_o;
pthread_mutex_t mutex_h;
pthread_cond_t cv_h;

// The function a hydrogen atom executes.  Pass in the identifier of the atom.

void *h_atom( void *arg )
{
	int i;
	int id = (int) arg;

	printf( "One H atom, id = %d\n", id );
	sleep( rand() % 5 ); // Sleep a random amount of time.

	// Before changing the state, protect the change by a pthread_mutex_lock
	// and pthread_mutex_unlock.

	pthread_mutex_lock( &lock_it );
	h_atoms++; // One more hydrogen atom has been created
	h[ id ] = 1; // Indicate that H atom with this id is available for use

	if( h_atoms >= 2 && o_atoms >= 1 )
	{
		// There are enough H and O atoms to make water.

		printf( "Inside H, enough H and O to make water, so water is made\n" );

		// Two H and one O atom is involved in making water.  Decrease the
		// state variables to indicate these atoms have been "used up".

		h_atoms -= 2;
		o_atoms--;

		// Now signal two H and one O atom that they can terminate, since
		// they have been involved in making water.  Specifically loop
		// through and find 2 H atoms and signal them explicitly.  Do the
		// same with one O atom.

		for( i = 0; i < total * 2 / 3; i++ )
			if( h[ i ] == 1 )
			{
				h[ i ] = 0;
				printf( "H atom %d signalling H atom %d\n", id, i );
				pthread_cond_signal( &cv_h );
				break;
			}

		for( i = 0; i < total * 2 / 3; i++ )
			if( h[ i ] == 1 )
			{
				h[ i ] = 0;
				printf( "H atom %d signalling H atom %d\n", id, i );
				pthread_cond_signal( &cv_h );
				break;
			}

		for( i = 0; i < total / 3; i++ )
			if( o[ i ] == 1 )
			{
				o[ i ] = 0;
				printf( "H atom %d signalling O atom %d\n", id, i );
				pthread_cond_signal( &cv_o );
				break;
			}
	}

	pthread_mutex_unlock( &lock_it );

	sleep( rand() % 5 ); // Sleep a random amount of time.

	// This is an H atom and so it needs to wait until it has been used in
	// a chemical reaction of making water.  Once it has, it can terminate.
	// Here, the thread waits on the condition that it's been used.

	pthread_mutex_lock( &mutex_h );
	while( h[ id ] != 0 )
		pthread_cond_wait( &cv_h, &mutex_h );
	pthread_mutex_unlock( &mutex_h );
}

// The function an oxygen atom executes.  Pass in the id of this atom.

void *o_atom( void *arg )
{
	int i;
	int id = (int) arg;

	// This function is virtually identical to the H atom function.  Only
	// differences will be highlighted.

	printf( "One O atom, id = %d\n", id );
	sleep( rand() % 5 );

	pthread_mutex_lock( &lock_it );
	o_atoms++; // This is an O atom, so the state changes to indicate this.
	o[ id ] = 1; // Indicate that this O atom is available for use.

	if( h_atoms >= 2 && o_atoms >= 1 )
	{
		printf( "Inside O, enough H and O to make water, so water is made\n" );
		h_atoms -= 2;
		o_atoms--;

		// As before, signal explicitly two H atoms and one O atom.  (We
		// know they will exist because the condition of the if statement
		// was satisfied.)

		for( i = 0; i < total * 2 / 3; i++ )
			if( h[ i ] == 1 )
			{
				h[ i ] = 0;
				printf( "O atom %d signalling H atom %d\n", id, i );
				pthread_cond_signal( &cv_h );
				break;
			}

		for( i = 0; i < total * 2 / 3; i++ )
			if( h[ i ] == 1 )
			{
				h[ i ] = 0;
				printf( "O atom %d signalling H atom %d\n", id, i );
				pthread_cond_signal( &cv_h );
				break;
			}

		for( i = 0; i < total / 3; i++ )
			if( o[ i ] == 1 )
			{
				o[ i ] = 0;
				printf( "O atom %d signalling O atom %d\n", id, i );
				pthread_cond_signal( &cv_o );
				break;
			}
	}

	pthread_mutex_unlock( &lock_it );

	sleep( rand() % 5 ); // Sleep a random amount of time.

	// This is an O atom.  It waits until is has been used in the chemical
	// reaction for making water.

	pthread_mutex_lock( &mutex_o );
	while( o[ id ] != 0 )
		pthread_cond_wait( &cv_o, &mutex_o );
	pthread_mutex_unlock( &mutex_o );
}

int main()
{
	int i;
	pthread_t *tid;
	unsigned int seed;

	// A seed value is required for the pseudo-random number generator
	// (since otherwise the same sequence of values will always be generated).

	printf( "Enter a seed value (an int) to randomize the number of H and O atoms created:\n" );
	scanf( "%d", &seed );

	srand( seed ); // Set the seed for the pseudo-random number generator

	// Get a random number of atom to create that is a multiple of 3.

	do{
		total = rand()% 50 + 1;
	}while( total % 3 != 0 );

	printf( "Main: a total of %d H and O atoms will be created.\n", total );

	// Allocate an array to hold enough thread identifiers for as many
	// atoms as we will create.

	tid = new pthread_t[ total ];

	// Allocate an array for as many H atoms and O atoms that we will create.
	// Initialize the array elements to zero, indicating that these atoms are
	// currently unused.

	h = new int[ total * 2 / 3 ];
	for( i = 0; i < total * 2 / 3; i++ )
		h[ i ] = 0;

	o = new int[ total / 3 ];
	for( i = 0; i < total / 3; i++ )
		o[ i ] = 0;

	// Initially, there are no H or O atoms in the system.

	h_atoms = o_atoms = 0;
	int no_o = 0;  // Used as identifier for O atom
	int no_h = 0;  // Used as identifier for H atom

	// Create twice as many H as O atoms.

	for( i = 0; i < total; i++ )
	{
		if( i % 3 != 0 )
		{
			// Now pass in an id for the H atom.

			pthread_create( &tid[ i ], NULL, h_atom, (void *) no_h );
			no_h++;
		}
		else
		{
			// Pass in an id for the O atom.

			pthread_create( &tid[ i ], NULL, o_atom, (void *) no_o );
			no_o++;
		}
	}

	// Wait for all the atoms to terminate, then the main thread will terminate.

	for( i = 0; i < total; i++ )
		pthread_join( tid[ i ], NULL );

	printf( "main() terminating\n" );
	return 0;
}
