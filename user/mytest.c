#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define SIG_10 10
#define SIG_TRY 11
void 
userHandler(int signum)
{
    printf("Hi everyone!\n");
}

// test the normal kill func with the known SIGKILL
void testKill(void){
    int npid,cstatus;
    if((npid = fork())>0){
        printf("entering to sleep ZZZ \n");
        sleep(10);
        kill(npid, SIGKILL);
        wait(&cstatus);
        printf("child is dead :( \n");
    }else{
        while(1){
            sleep(5);
        }
    }
}

// test the blocking SIGCONT and SIGSTOP, 
// SIGSTOP should have no effects while SIGCONT needs to be blocked, 
// expected result: print the child, stop, and then kill itself. 
void testStopCont(void){
    int npid,cstatus;
    int mask = (1 << SIGCONT) | (1 << SIGSTOP);

    sigprocmask(mask);

    if((npid = fork())>0){
        sleep(20);
        kill(npid, SIGSTOP);
        printf("child is stopped :O \n");
        sleep(30);
        kill(npid, SIGCONT);
        printf("child try to continue\n");
        sleep(40);
        kill(npid, SIGKILL);
        printf("killing the child\n");

                wait(&cstatus);
        printf("cstat:%d\n",cstatus);
    }else{
        while(1){
            sleep(5);
            printf("child is sleeping \n");
        }
    }
}

// test at first the sigaction function were the function userHandler enters. 
// send userHandler to the child pending signals
// chiled enters to sleep 
void
userSignals(){
    struct sigaction sig;

    memset(&sig,0,sizeof(struct sigaction));
    sig.sa_handler = &userHandler;
    sig.sigmask = 1;

    sigaction(SIG_10, &sig, 0); 
    int npid = fork();
    if(npid > 0){
        sleep(10);
        kill(npid, SIG_10);
        printf("sig_10 sent to process %d \n", npid);
    }
    else{
        printf("child is entering to sleep ZZZ \n");
        sleep(30);
    }
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
        kill(c_pid, SIG_TRY);
        printf("sent sig_try\n");
    }
    else{
        printf("I'm the first child\n");
        sleep(30);
    }

}
struct test {
    void (*f)(void);
    char *s;
  } tests[] = {
     //{testKill, "kill"},
     //{testStopCont, "stop & cont"},
     {userSignals, "sigaction & user sign"},
    { 0, 0}, 
  };
  int main(void){
    for (struct test *t = tests; t->s != 0; t++) {
        printf("----------- test - %s -----------\n", t->s);
        t->f();
    }

    exit(0);
    }

