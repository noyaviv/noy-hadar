typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;
//2.1.1 changes: 
#define null 0 
#define SIGSNUM 32
#define SIG_DFL 0 /* default signal handling */
#define SIG_IGN 1 /* ignore signal */
#define SIGKILL 9
#define SIGSTOP 17
#define SIGCONT 19

//2.1.4 new struct required for sigaction
// struct sigaction {
// void (*sa_handler) (int);
// uint sigmask; }; 

