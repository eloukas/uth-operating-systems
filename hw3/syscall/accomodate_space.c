#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

int main( int argc, char *argv[] ){

	int *pointer;
    int i;
	pointer = (int*)malloc(sizeof(int)*1337);

	//Declare 5 arrays of 'char' variables
	char *space0, *space1, *space2, *space3, *space4;

	clock_t begin = clock();
	//Accomodate some (random) space for them
	space0 = (char*) malloc (sizeof(char) *333333);
    space1 = (char*) malloc (sizeof(char) *44444);
	space2 = (char*) malloc (sizeof(char) *3214);
	space3 = (char*) malloc (sizeof(char) *10000000);
	space4 = (char*) malloc (sizeof(char) *55630);

	
	//Do this 100 times
	for(i=0; i < 100; i++){
		
		//Attempt to resize the memory block pointed to by 'pointer' for i*1337 bytes - big size
		pointer = (int*)realloc(pointer,(i*1337) );
		
		sleep(2); // not needed 

	}
	
	clock_t end = clock();
	
	//return time in an internal scale called "clocks"
	double time_spent = (double)(end-begin) / CLOCKS_PER_SEC;
	printf("%Time elapsed with the current algorithm is %f ", time_spent);


	return 0;
}
