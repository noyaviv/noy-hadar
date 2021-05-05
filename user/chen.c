
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/syscall.h"


// #define NULL ((void *)0)
#define SIG_DFL 0 /* default signal handling */
#define SIG_IGN 1 /* ignore signal */
#define SIGKILL 9
#define SIGSTOP 17
#define SIGCONT 19


// struct sigaction {
//   void (*sa_handler)(int);
//   uint sigmask;
// };




// Test 7 - the test

void chentest(){
    int pid;
    pid = fork();
   

    if(pid!=0)
    {  
        printf("pid of son parent is:%d\n",pid);
        sleep(50);
        kill(pid,17);
        printf("sending SIGSTOP done");
        sleep(50);
        kill(pid,19);
        printf("sending SIGCONT done");
        sleep(50);

        for(int i=0;i<1000;i++)
        {
            printf("#");
        }

        while(1)
        {
            kill(pid,9); //SIGKILL
            printf(".");
           
        }
    }

    else{
        while(1)
        {
            printf("|");
        }
       
    }

    exit(0);
}

#define SIG_PRINT 3



void mask_test(){
    uint newmask = (1 << SIG_PRINT);
    uint oldmask = 0;
    int pid ,  status = 0;
   
    if((pid = fork()) == 0){
        //child

        oldmask = sigprocmask(newmask);
        printf("old mask: %d\n", oldmask);
        printf("current mask: %d\n", newmask);

        sleep(40);
        printf("Child woke up\n");

        newmask = 0;
        oldmask = sigprocmask(newmask);

    }
    else{ //parent

        sleep(10);
        kill(pid, SIG_PRINT); // should be blocked, and in pending
        printf("Parent sent kill signal to child , pid: %d\n", pid);

        wait(&status);
        printf("exit\n");        
    }

    /* test output:
        old mask: 0
        current mask: 8
        Parent sent kill signal to child , pid: %d
        Child woke up
        exit
                    */

   
}

void kernelhandlers_test(){
    int pid , status = 0 ;
   
    printf("Test No.1 : \n");
    // SIG_DFL check & sigprocmask
    if((pid = fork()) == 0){
        printf("child pid %d is entering sleep mode\n", pid); //TODO : remove 
        //child1
        while(1){
            printf("child\n");
            sleep(5);
        }
    }
    else{ //parent
        printf("perent pid %d is entering sleep mode\n", pid); //TODO : remove 
        sleep(10);
        printf("perent pid %d is entering kill with sig STOP\n", pid); //TODO : remove 
        kill(pid,SIGSTOP);
        sleep(10);
        printf("perent pid %d is entering kill with sig CONT\n", pid); //TODO : remove 
        kill(pid,SIGCONT);
        sleep(10);
        printf("perent pid %d is entering kill with sig KILL\n", pid); //TODO : remove 
        kill(pid,SIGKILL);
        printf("waiting for child to finish\n");
        wait(&status);
        printf("OK\n");      
    }

// ===================================
    // block kill test

    printf("\nTest No.2: \n");

    if((pid = fork()) == 0){
        //child:
        sigprocmask(1 << SIGKILL);
        kill(getpid(),SIGKILL);
        printf("kill signal was blocked, error!!!\n");
    }else{
        wait(&status);
        printf("kill signal was not blocked --> OK\n");
    }

// ================================
    // change SIGSTOP handler test

    printf("\nTest No.3: \n");

    int ret = sigaction(SIGSTOP, (void*) SIGKILL, 0);

    if(ret == -1){
        printf("Could not change handler for SIGSTOP --> OK\n");
    }
    else
        printf("Error! SIGSTOPs handler has been changed\n");
 
}


int handler_print(int signum){
    printf("exec. handler_print for signal: %d \n", signum);
    return 2;
}

int handler_print2(int signum){
    printf("exec. handler_print for signal: %d \n", signum);
    return 2;
}

int handler_print3(int signum){
    printf("exec. handler_print for signal: %d \n", signum);
    return 2;
}




void userhandler_test(){
   
    printf("handler_print address: %d\n",handler_print);
    printf("handler_print2 address: %d\n",handler_print2);
    printf("handler_print3 address: %d\n",handler_print3);

    struct sigaction handler = {
        (void*)handler_print2, 1
    };



    sigaction(SIG_PRINT, &handler, 0);
    kill(getpid(), SIG_PRINT);
   
}

//Maayan test:

    
    
void userfun(int num){
    fprintf(2, "user function , signum: %d\n", num);
}

void blockedfun(int num){
    fprintf(2, "The second is Blocked! I'm signum: %d\n", num);
}

void test2(){
    fprintf(2,"userfun address: %p\n",(void*)userfun);
    fprintf(2,"blockedfun address: %p\n",(void*)blockedfun);
    int pid = fork();
    if (pid == 0){
        struct sigaction sigact = {userfun,0};
        struct sigaction blocking = {blockedfun,5};

        sigaction(2, &blocking, (void*) 0);
        sigaction(5, &sigact, (void*) 0);
        //sigprocmask(1<<5);
        fprintf(2, "child sleep\n");
        while(1){

        }; //wait for parent to send the signal
        exit(0);
    }
    else{
       
        fprintf(2, "parent is sleeping\n");
        sleep(100);
        fprintf(2, "parent sending signals\n");
        kill(pid, 2);
        sleep(100);
        kill(pid, 5);
        sleep(100);
        kill(pid,9);
        wait(0);
    }
}


int main(){


    kernelhandlers_test(); // test kill function and kernel handlers
    //mask_test();           // test block and sigprocmask
    // userhandler_test();        
    // chentest();
    // test2();
	exit(0);
}


