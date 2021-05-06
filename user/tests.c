#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

#include "kernel/spinlock.h"  // NEW INCLUDE FOR ASS2
#include "kernel/proc.h"         // NEW INCLUDE FOR ASS 2, has all the signal definitions and sigaction defi

#define SIG_TRY 11

// struct sigaction {
//     void (*sa_handler) (int);    //sa_handler is the signal handler
//     uint sigmask;                //sigmask specifies a mask of signals which should be blocked during the execution of the signal handler.
// };

void
test_kernel_signals(){
     int c_pid = fork();
    if(c_pid > 0){
        sleep(3);
        kill(c_pid, SIGKILL);
        int c_kill;
        if((c_kill = fork()) > 0){
            kill(c_kill, SIGKILL);
        }
        else{
            sleep(10);
            printf("Shouldn't get here!\n");
        }



        if((c_pid=fork()) > 0){
            kill(c_pid, SIGSTOP);
            sleep(2);
            kill(c_pid, SIGKILL);
        }
        else{
            sleep(1);
            printf("Still alive : child 2!\n");
            kill(c_pid, SIGCONT);
            sleep(5);
            printf("Shouldn't get here 2\n");
            sleep(5);
        }

    }
    else{
        printf("pid %d :I am child 1!\n", getpid());
        sleep(7);
    }
}


void 
print_handler(int signum)
{
    printf("Hey dude!\n");
}

void
print_bye_handler(int signum)
{
    printf("bye\n");
}

void
print_number_handler(int signum)
{
    printf("number\n");
}

void
test_user_signals(){
    struct sigaction signal;
    memset(&signal,0,sizeof(struct sigaction));
    signal.sa_handler = &print_number_handler;
    signal.sigmask = 1;
    printf("handler1: %d, handler2: %d handler3: %d\n",&print_handler,&print_bye_handler,&print_number_handler);
    printf("user signal: %d, handler: %d mask: %d\n",signal, signal.sa_handler, signal.sigmask);
    sigaction(SIG_TRY, &signal, 0);
    int c_pid = fork();
    if(c_pid > 0){
        sleep(10);
        // kill(c_pid, SIGKILL);
        kill(c_pid, SIG_TRY);
        printf("sent sig_try\n");
    }
    else{
        printf("I'm the first child\n");
        sleep(30);
        // printf("still here\n");
    }
}

int wait_sig = 0;

void test_handler(int signum){
    wait_sig = 1;
    printf("Received sigtest\n");
}

void user_tests_signal(){
    int pid;
    int testsig=15;
    struct sigaction act = {test_handler, (uint)(1 << 29)};
    struct sigaction old;
    // printf("test handler: %d\n", test_handler);
    // printf("handler: %d mask: %d\n", act.sa_handler, act.sigmask);

    // printf("here1\n");
    sigprocmask(0);
    // printf("here2\n");
    sigaction(testsig, &act, &old);
    // printf("here3\n");
    if((pid = fork()) == 0){
        while(!wait_sig)
            sleep(1);
        exit(0);
    }
    // printf("here4\n");
    kill(pid, testsig);
    // printf("here5\n");
    wait(&pid);
    printf("Finished testing signals\n");
}

int
main(int argc, char *argv[]){
    //test_kernel_signals();
    test_user_signals();
    //user_tests_signal();
    exit(0);
}