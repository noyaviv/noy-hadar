#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

//User defined sigals
#define SIG_2 2
#define SIG_3 3
#define SIG_4 4

//User defined handlers
void handler2(int i){
    sleep(2);
    printf("SIG_2 handled by %d\n",getpid());
}
void handler3(int i){
    sleep(2);
    printf("SIG_3 handled by %d with i=%d\n",getpid(),i);
}
void test_kill(void){
    int child_pid,cstatus;
    if((child_pid = fork())>0){
        sleep(30);
        kill(child_pid, SIGKILL);
        wait(&cstatus);
        printf("killed child!\n");
    }else{
        while(1){
            sleep(4);
        }
    }
}
// expected result: print child, stops, cotinue, and killed.
void test_stop_cont(void){
    int child_pid,cstatus;
    if((child_pid = fork())>0){
        printf("(from parent) child pid is %d \n" ,child_pid); //TODO delete
        sleep(30);
        kill(child_pid, SIGSTOP);
        printf("sigstop sent to child\n");
        sleep(40);
        kill(child_pid, SIGCONT);
        printf("sigcont sent to child\n");
        sleep(30);
        kill(child_pid, SIGKILL);
        printf("killed child\n");
        wait(&cstatus);
        printf("cstat:%d\n",cstatus);
    }else{
        printf("(from child) child pid is %d \n" ,child_pid);//TODO delete
        while(1){
            sleep(4);
            printf("child still sleeping!\n");
        }
    }
}
// expected result: print child, stops, cotinue is blocked , and killed.
// verifying blocking sigkill and sigcont is impossible +  ignore sigcont
void test_stop_cont_2(void){
    int child_pid,cstatus;
    int mask = (1 << SIGCONT) | (1 << SIGKILL) | (1 << SIGSTOP);
    sigprocmask(mask);

    if((child_pid = fork())>0){
        sleep(30);
        kill(child_pid, SIGSTOP);
        printf("sigstop sent to child\n");
        sleep(30);
        kill(child_pid, SIGCONT);
        printf("sigcont sent to child (should be ignored)\n");
        sleep(40);
        kill(child_pid, SIGKILL);
        printf("killed child\n");

                wait(&cstatus);
        printf("cstat:%d\n",cstatus);
    }else{
        while(1){
            sleep(4);
            printf("child still sleeping!\n");
        }
    }
}
// expected result: print child, stops, cotinue is blocked , and killed.
// verifying blocking sigkill and sigcont is impossible +  ignore sigcont
void test_stop_cont_3(void){
    int child_pid,cstatus;
    // int mask = (1 << SIGCONT) | (1 << SIGKILL) | (1 << SIGSTOP);

   if((child_pid = fork())>0){
        sleep(30);
        kill(child_pid, SIGSTOP);
        printf("sigstop sent to child\n");
        sleep(30);
        kill(child_pid, SIGCONT);
        printf("sigcont sent to child (should be ignored)\n");
        sleep(40);
        kill(child_pid, SIGKILL);
        printf("killed child\n");
        wait(&cstatus);
        printf("cstat:%d\n",cstatus);
    }else{
        int si = SIG_IGN;
        void* p;
        sigaction(SIGCONT,(struct sigaction*)&si,(struct sigaction*)&p);
        printf("p:%d\n",p);
        while(1){

            sleep(4);
            printf("child still sleeping!\n");
        }
    }
}

//verifying successful handler's behaviour changed + handling user handler
void test_sigaction(void){
    int cpid,cstatus;
    if((cpid = fork())>0){
        sleep(40);
        kill(cpid,SIG_2);
        sleep(5);
        kill(cpid,SIG_3);
        sleep(5);
        sleep(10);
        kill(cpid,SIG_4);

        wait(&cstatus);
        printf("cstat:%d\n",cstatus);
    }else{
        struct sigaction sa;
        sa.sa_handler = handler3;
        sa.sigmask = 0;
        sigaction(SIG_2,&sa,0);
               sigaction(SIG_3,&sa,0);
        while(1){
            sleep(10);
            printf("Child ");
        }
    } 
}
//verifying successful handler's behaviour changed + handling user handler
// verify handlers goes to child proc in fork
void test_sigaction_2(void){
    int cpid,cstatus;

    struct sigaction sa;
    sa.sa_handler = handler3;
    sa.sigmask = 0;
       sigaction(SIG_2,&sa,0);
    sigaction(SIG_3,&sa,0);

    if((cpid = fork())>0){
        sleep(40);
        kill(cpid,SIG_2);
        sleep(5);

        kill(cpid,SIG_3);
        sleep(5);
        kill(cpid,SIG_2);

        sleep(10);
        kill(cpid,SIG_4);

        wait(&cstatus);
        printf("cstat:%d\n",cstatus);
    }else{
        while(1){
            sleep(10);
            printf("Child ");
        }
    } 
}
struct test {
    void (*f)(void);
    char *s;
  } tests[] = {
     //{test_kill, "test_kill"},
     //{test_stop_cont, "test_stop_cont"},
     {test_stop_cont_2, "test_stop_cont_2"},
    //{test_stop_cont_3, "test_stop_cont_3"},
     //{test_sigaction, "test_sigaction"},
     //{test_sigaction_2, "test_sigaction_2"},
    { 0, 0}, 
  };
  int main(void){
    for (struct test *t = tests; t->s != 0; t++) {
        printf("----------- test - %s -----------\n", t->s);
        t->f();
    }

    exit(0);
    }