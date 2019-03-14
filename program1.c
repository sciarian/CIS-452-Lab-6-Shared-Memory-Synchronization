/**
 * CIS 452 20: Lab 6 : Shared Memory Synchronization.
 * By: Anthony Sciarini and Santiago Quirogas.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>

#define SIZE 16

int main (int argc, char *argv[]) {

    //CREATE VAR
    int status;
    long int i, loop, temp, *shmPtr;
    int shmId;
    pid_t pid;

    key_t semkey;
    int semid;
    struct sembuf sbuf[2];

    //READ IN CMD ARGS
    if(argc > 1){
	loop = (int) atoi(argv[1]);
    	printf("\nThe size of the loop is: %ld\n", loop);
    }else{
	printf("You forgot to input a parameter :(");
    	exit(0);
    }

    //CREATE AND INTIALIZE SHARED MEMORY
    if ((shmId = shmget (IPC_PRIVATE, SIZE,
                         IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
        perror ("i can't get no..\n");
        exit (1);
    }

    if ((shmPtr = shmat (shmId, 0, 0)) == (void *) -1) {
        perror ("can't attach\n");
        exit (1);
    }

    shmPtr[0] = 0;
    shmPtr[1] = 1;

    /////////////////////
    //CREATE SEMAPHORES//
    /////////////////////

    //Creat a unique key for the semaphore
    if ((semkey = ftok("/tmp", 'a')) == (key_t) -1){
		perror("IPC error: ftok");
		exit(1);
    }

    //Get semaphore assosciated with the semkey
    if ((semid = semget(semkey,0,0)) == -1){
	    //If the semaphore has not been created yet
	    if((semid = semget(semkey, 1,  IPC_CREAT | S_IRUSR | S_IWUSR)) != -1){
		//Intialize semaphore
		semctl(semid, 0, SETVAL, 1);
	    }

    }
    


    if (!(pid = fork ())) {
        for (i = 0; i < loop; i++) {

	     //Wait for semval 0 and increment by 1
	     sbuf[0].sem_num  = 0;
	     sbuf[0].sem_op   = 0;//Wait for semval == 0
	     sbuf[0].sem_flg = 0;
	     sbuf[1].sem_num  = 0;
	     sbuf[1].sem_op   = 1;//Increment semval
	     sbuf[1].sem_flg = 0; 
	
	     if( semop(semid, sbuf, 2) == -1){
	     	perror("SEMOP failed");
		exit(1);
	     }

	
	     //////CRITICAL SECTION BEGIN.//////
             temp      = shmPtr[0];
	     shmPtr[0] = shmPtr[1];
	     shmPtr[1] = temp;
	     //////CRITICAL SECTION END.//////

	     //Decrement semval
	     sbuf[0].sem_num  =  0;
	     sbuf[0].sem_op   = -1;//Decrement semval (Signal)
	     sbuf[0].sem_flg =  0;
	     	     
 	     if( semop(semid, &sbuf[0], 1) == -1){
	      	perror("SEMOP failed");
	  	exit(1);
	     }
	}

        if (shmdt (shmPtr) < 0) {
            perror ("just can 't let go\n");
            exit (1);
        }
        exit (0);
    }
    else {

    //Parent waits first (sends a signal first)
    //Decrement semval
    sbuf[0].sem_num  =  0;
    sbuf[0].sem_op   = -1;//Decrement semval (Signal)
    sbuf[0].sem_flg =  0;
     
    if( semop(semid, &sbuf[0], 1) == -1){
     	perror("SEMOP failed");
	exit(1);
    }
   
    for (i = 0; i < loop; i++) {
	     //Wait for semval 0 and increment by 1
	     sbuf[0].sem_num  = 0;
	     sbuf[0].sem_op   = 0;//Wait for semval == 0
	     sbuf[0].sem_flg = 0;
	     sbuf[1].sem_num  = 0;
	     sbuf[1].sem_op   = 1;//Increment semval
	     sbuf[1].sem_flg = 0; 

	     if( semop(semid, sbuf, 2) == -1){
	     	perror("SEMOP failed");
		exit(1);
	     }

	     //////CRITICAL SECTION BEGIN.//////
	     temp      = shmPtr[1];
	     shmPtr[1] = shmPtr[0];
	     shmPtr[0] = temp;
	     //////CRITICAL SECTION END.  //////

	     //Decrement semval
	     sbuf[0].sem_num  =  0;
	     sbuf[0].sem_op   = -1;//Decrement semval (Signal)
	     sbuf[0].sem_flg =  0;
	     
 	    if( semop(semid, &sbuf[0], 1) == -1){
	     	perror("SEMOP failed");
		exit(1);
	    }
	}
    }

    wait (&status);
    printf ("values: %li\t%li\n", shmPtr[0], shmPtr[1]);


    //DEALLOCATE SHARED MEMORY
    if (shmdt (shmPtr) < 0) {
        perror ("just can't let go\n");
        exit (1);
    }
    if (shmctl (shmId, IPC_RMID, 0) < 0) {
        perror ("can't deallocate\n");
        exit (1);
    }

    //DEALLOCATE SEMAPHORE
    if(semctl (semid, 0, IPC_RMID) > 0){
	perror("SEMAFAILURE!!!! (>o_o)>)");
	exit(1);
    } 

    return 0;
}
