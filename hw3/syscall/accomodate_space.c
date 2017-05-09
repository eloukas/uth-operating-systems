#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main( int argc, char *argv[] ){

	int *pointer;
    int i;
	pointer = (int*)malloc(sizeof(int)*1337);

	//Declare 5 arrays of 'char' variables
	char *space0, *space1, *space2, *space3, *space4;

	//Accomodate some (random) space for them
	space0 = (char*) malloc (sizeof(char) *333333);
    space1 = (char*) malloc (sizeof(char) *44444);
	space2 = (char*) malloc (sizeof(char) *3214);
	space3 = (char*) malloc (sizeof(char) *10000000);
	space4 = (char*) malloc (sizeof(char) *55630);

	
	//Do this 100 times
	for(i=0; i < 100; i++){
		
		//Attempt to resize the memory block pointed to by 'pointer' for i*1337 bytes
		pointer = (int*)realloc(pointer,(i*1337) );
		
		sleep(2); // not needed 

	}


	return 0;
}
