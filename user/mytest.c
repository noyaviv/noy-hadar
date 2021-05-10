#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define SIG_10 10
#define SIG_USER 11

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

void
userHandler(int signum)
{
    printf("number\n");
}

// test at first the sigaction function were the function userHandler enters. 
// send userHandler to the child pending signals
// chiled enters to sleep 
void
userSignals(){
    struct sigaction signal;
    memset(&signal,0,sizeof(struct sigaction));
    signal.sa_handler = &userHandler;
    signal.sigmask = 1;

    sigaction(SIG_USER, &signal, 0);
    int npid = fork();
    if(npid > 0){
        sleep(10);
        kill(npid, SIG_USER);
        printf("sent SIG_USER\n");
    }
    else{
        printf("child is entering to sleep ZZZ \n");
        sleep(30);
    }

}


struct test {
    void (*f)(void);
    char *s;
  } tests[] = {
     {testKill, "kill"},
     //{testStopCont, "stop & cont"},
     //{userSignals, "sigaction & user sign"},
    { 0, 0}, 
  };
  int main(void){
    for (struct test *t = tests; t->s != 0; t++) {
        printf("----------- test - %s -----------\n", t->s);
        t->f();
    }

    exit(0);
    }

