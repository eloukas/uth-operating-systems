#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

int main( int argc, char *argv[] ){



	char * a ;
	int i ;

	for( i = 0; i < 10000; i++)
	{
	    a = (char*) malloc(20*sizeof(char)) ;
	    if(a == NULL) printf("NULL\n") ;
	}
	
	//clock_t end = clock();
	
	//return time in an internal scale called "clocks"
	//double time_spent = (double)(end-begin) / CLOCKS_PER_SEC;
	//printf("%Time elapsed with the current algorithm is %f ", time_spent);


	return 0;
}
