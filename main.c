
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <limits.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <string.h>

#include <pthread.h>

#include <time.h>
#include <omp.h>

#define N_PROCESSES 10

#define DUMMY_PRINT 0

#define N_SCHED_DEFAULT 0

#define N_SCHED_OTHER 0

#define N_SCHED_FIFO_P90 4
#define N_SCHED_FIFO_P80 0

//#define FIFO_WITH_IO
#define IO_SIZE 1000000
#define IO_NUM 110

#define N_SCHED_RR_P90 0
#define N_SCHED_RR_P80 0

#define N_SCHED_DEADLINE 1

#define SCHED_DEADLINE 6

double global_start;

#ifdef __x86_64__
#define __NR_sched_setattr           314
#define __NR_sched_getattr           315
#endif

#ifdef __i386__
#define __NR_sched_setattr           351
#define __NR_sched_getattr           352
#endif

struct sched_attr {
    __u32 size;              /* Size of this structure */
    __u32 sched_policy;      /* Policy (SCHED_*) */
    __u64 sched_flags;       /* Flags */
    __s32 sched_nice;        /* Nice value (SCHED_OTHER,
                                SCHED_BATCH) */
    __u32 sched_priority;    /* Static priority (SCHED_FIFO,
                                SCHED_RR) */
    /* Remaining fields are for SCHED_DEADLINE */
    __u64 sched_runtime;
    __u64 sched_deadline;
    __u64 sched_period;
};


int sched_setattr(pid_t pid,
               const struct sched_attr *attr,
               unsigned int flags)
{
     return syscall(__NR_sched_setattr, pid, attr, flags);
}


void save_data( char* filename, char *data){
    FILE *h = fopen( filename  , "w");
    if (h){
        fwrite( data, sizeof(char), strlen(data), h);
        fclose( h );
    }
}


void CPU_intensive(int dummy_print){

    for ( int i = 0; i < INT_MAX; i ++ ){

    }

}

void IO_intensive(int dummy_print){
    
    char dummy_buffer[ IO_SIZE ];
    FILE *h = fopen("/tmp/dummy.dat", "r");

    if ( h == NULL ){
        perror("");
        return;
    }

    for ( int i = 0; i < INT_MAX; i ++ ){

        if ( ( i  % (INT_MAX/ IO_NUM ) ) == 0 ){

            int nbytes = fread( dummy_buffer, sizeof(char), IO_SIZE, h );

            if (nbytes != (IO_SIZE*sizeof(char))){
                // oops
                break;
            }
        }
        
    }

    fclose( h );
}

int print_sched(int policy){
    switch (policy){
        case SCHED_FIFO:
            printf("SCHED_FIFO");
            break;
        case SCHED_RR:
            printf("SCHED_RR");
            break;
        case SCHED_OTHER:
            printf("SCHED_OTHER");
            break;
        default:
            printf("Unknown scheduler policy: %d", policy);
            break;
    }
}

void func_sched_default( int id , void (*foo)(int));
    
void func_sched_other( int id , void (*foo)(int));

void func_sched_fifo( int id, int prio, void (*foo)(int) );

void func_sched_rr( int id, int prio, void (*foo)(int) );

void func_sched_deadline( int id, void (*foo)(int) );  

int main(int argc, char *argv[]){

    pid_t processes[ N_PROCESSES ];
    pid_t p_pid, other_pid;

    int cur_process = 0;
    int retval;

    char filename[50];
    char data[50];

#ifdef FIFO_WITH_IO
    if ( other_pid = fork() ){

        int filesize = IO_SIZE * IO_NUM;

        char cmd[ 100 ];

        //sprintf(cmd, "", filesize);
        //printf("%s", cmd);

        execv( "/usr/bin/dd if=/dev/urandom of=/tmp/dummy.dat bs=100M count=1" , argv );

    } else {
        int status;
        wait( &status );

        printf("Status: %d", status);
    }
#endif

    sprintf(data , "%ld", CLOCKS_PER_SEC);
    save_data( "clocks_per_sec.out", data);

    for (int i = 0; i < N_PROCESSES; i++){
        processes[ i ] = 0;
    }

    global_start = omp_get_wtime();

    for (int p = 0; p < N_SCHED_DEFAULT ; p++){
        func_sched_default( p , CPU_intensive);
    }

    for (int p = 0; p < N_SCHED_OTHER ; p++){
        func_sched_other( p , CPU_intensive);
    }

    for ( int p = 0; p < N_SCHED_FIFO_P90; p ++ ){
#ifdef FIFO_WITH_IO
        func_sched_fifo( p, 90, IO_intensive );
#else        
        func_sched_fifo( p, 90, CPU_intensive );
#endif
    }    

    for ( int p = 0; p < N_SCHED_FIFO_P80; p ++ ){
#ifdef FIFO_WITH_IO
        func_sched_fifo( p, 80, IO_intensive );
#else        
        func_sched_fifo( p, 80, CPU_intensive );
#endif
    }

    for ( int p = 0; p < N_SCHED_RR_P90; p ++ ){
        func_sched_rr( p, 90, CPU_intensive );
    }

    for ( int p = 0; p < N_SCHED_RR_P80; p ++ ){
        func_sched_rr( p, 80, CPU_intensive );
    }

    for ( int p = 0; p < N_SCHED_DEADLINE; p ++ ){
        func_sched_deadline( p, CPU_intensive );
    }        
    
    execv( "/usr/bin/top", argv );

}



void func_sched_default( int id , void (*fn)(int) ){

    pid_t pid;

    if ( pid = fork() ){

        char pname[ 16 ];
        int retval;

        sprintf( pname, "sched_default%d", id);

        retval = prctl( PR_SET_NAME, pname );

        if ( retval < 0 ){
            perror("");
            exit(retval);
        }

        printf("sched_default%d: ", id );
        print_sched( sched_getscheduler(0) );

        clock_t start = clock();
        fn( DUMMY_PRINT );
        double end = omp_get_wtime();
        
        char filename[50];
        sprintf( filename, "sched_default%d_time.out", id);

        char data[50];
        sprintf( data, "%.6f\n", end - global_start );

        save_data( filename, data );

        exit( 0 );
    }
}


void func_sched_deadline( int id , void (*fn)(int) ){

    pid_t pid;

    if ( pid = fork() ){

        char pname[ 16 ];
        int retval;

        sprintf( pname, "sched_deadline%d", id);

        retval = prctl( PR_SET_NAME, pname );

        if ( retval < 0 ){
            perror("SCHED_DEADLINE");
            exit(retval);
        }

        struct sched_attr attr;

        unsigned int flags = 0;

        // printf("deadline thread started [%ld]\n", gettid());

        attr.size = sizeof(attr);
        attr.sched_flags = 0;
        attr.sched_nice = 0;
        attr.sched_priority = 0;

        /* This creates a 10ms/30ms reservation */
        attr.sched_policy = SCHED_DEADLINE;
        attr.sched_runtime = 10 * 1000 * 1000;
        attr.sched_period = attr.sched_deadline = 30 * 1000 * 1000;

        //attr.sched_runtime = 15 * (long int) ( 1000 * 1000 * 1000 ) ;
        //attr.sched_period = attr.sched_deadline = 30 * (long int)(1000 * 1000 * 1000);

        retval = sched_setattr(0, &attr, flags);

        if ( retval != 0){
            perror("sched_setattr");
            exit(retval);
        }

        printf("sched_deadline%d: ", id );
        print_sched( sched_getscheduler(0) );

        fn( DUMMY_PRINT );
        double end = omp_get_wtime();

        char filename[50];
        sprintf( filename, "sched_deadline%d_time.out", id);

        char data[50];
        sprintf( data, "%.6f\n", end - global_start );

        save_data( filename, data );   

        exit( 0 );
    }
}


void func_sched_other( int id , void (*fn)(int) ){

    pid_t pid;

    if ( pid = fork() ){

        char pname[ 16 ];
        int retval;

        sprintf( pname, "sched_other%d", id);

        retval = prctl( PR_SET_NAME, pname );

        if ( retval < 0 ){
            perror("");
            exit(retval);
        }

        struct sched_param param;

        param.sched_priority = 0;

        retval = sched_setscheduler( 0 , SCHED_OTHER, &param);

        if (retval != 0 ){
            perror("");
            exit(retval);
        }

        printf("sched_other%d: ", id );
        print_sched( sched_getscheduler(0) );

        clock_t start = clock();
        fn( DUMMY_PRINT );
        double end = omp_get_wtime();

        char filename[50];
        sprintf( filename, "sched_other%d_time.out", id);

        char data[50];
        sprintf( data, "%.6f\n", end - global_start );

        save_data( filename, data );        

        exit( 0 );    
    }
}

void func_sched_fifo( int id, int prio, void (*fn)(int)){

    pid_t pid;

    if ( pid = fork()  ){

        int retval;

        char pname[ 16 ];

        sprintf( pname, "sched_fifo%d_p%d", id, prio);

        retval = prctl( PR_SET_NAME, pname );

        if ( retval < 0 ){
            perror("");
            exit(retval);
        }

        struct sched_param param;

        param.sched_priority = prio;

        retval = sched_setscheduler( 0 , SCHED_FIFO, &param);

        if (retval != 0 ){
            perror("");
            exit(retval);
        }

        printf("sched_fifo%d: ", id);
        print_sched( sched_getscheduler(0) );        

        clock_t start = clock();
        fn( DUMMY_PRINT );
        double end = omp_get_wtime();

        char filename[50];
        sprintf( filename, "sched_fifo%d_p%d_time.out", id, prio);

        char data[50];
        sprintf( data, "%.6f\n", end - global_start );

        save_data( filename, data );

        exit( 0 );
    }
}

void func_sched_rr( int id, int prio, void (*fn)(int)){

    pid_t pid;

    if ( pid = fork()  ){

        int retval;

        char pname[ 16 ];

        sprintf( pname, "sched_rr%d_p%d", id, prio);

        retval = prctl( PR_SET_NAME, pname );

        if ( retval < 0 ){
            perror("");
            exit(retval);
        }

        struct sched_param param;

        param.sched_priority = prio;

        retval = sched_setscheduler( 0 , SCHED_RR, &param);

        if (retval != 0 ){
            perror("");
            exit(retval);
        }

        struct timespec tp;

        retval = sched_rr_get_interval(0, &tp);
        if (retval){
            perror("[sched_rr] ERROR: ");
            return;
        } 

        printf("sched_rr%d: ", id);
        print_sched( sched_getscheduler(0) );        

        clock_t start = clock();
        fn( DUMMY_PRINT );
        double end = omp_get_wtime();

        char filename[50];
        char data[50];

        sprintf( filename, "sched_rr%d_p%d_time.out", id, prio);        
        sprintf( data, "%.6f\n", end - global_start );

        save_data( filename, data );

        sprintf( filename, "sched_rr%d_p%d_quantum.out", id, prio);
        sprintf( data, "sched_rr%d_p%d: quantum = %ld ns", id, prio, tp.tv_nsec);

        save_data( filename, data );                

        exit( 0 );
    }
}