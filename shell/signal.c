#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

int main (int argc, char *argv[])
{
  /*
   Declare a new segset mask we will fill with the
   signals we want to ignore
  */
  sigset_t newmask;

  /*
    Empty out the sigset.  We can't call memset as it's
    not guaranteed to clear the set
  */
  sigemptyset( &newmask );

  /*
    Add SIGINT to the set.  This will allow us to ignore
    any ctrl-c presses
  */
  sigaddset( &newmask, SIGINT );
  sigaddset( &newmask, SIGTSTP );

  /*
    Finally, add the new signal set to our process' control block
    signal mask element.  This will block all SIGINT signals from
    reaching the process
  */
  if( sigprocmask( SIG_BLOCK, &newmask, NULL ) < 0 )
  {
    perror("sigprocmask ");
  }  

  while (1) {
    sleep (1);
  }
  
  return 0;
}
