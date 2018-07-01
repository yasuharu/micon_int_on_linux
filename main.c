#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "ringbuf.h"

// -----
// Micon program
// -----
void register_irq_handler(void (*func)(void));

void uart_rx_handler()
{
  static int     pos   = 0;
  static uint8_t buf[] = "test";

  ringbuf_push(buf[pos]);

  pos++;
}

void micon_main()
{
  register_irq_handler(uart_rx_handler);

  while(1)
  {
    if(ringbuf_length() > 0)
    {
      printf("%d\n", ringbuf_pop());
    }
  }
}

// -----
// Control program
// -----
pthread_t thread_handle;
void      (*irq_handler)(void);

enum INT_STATUS
{
  INT_STATUS_READY,
  INT_STATUS_SIGUSR1_SETUP,
  INT_STATUS_RUN,
  INT_STATUS_FIN,
};
volatile int int_status = 0;

void register_irq_handler(void (*func)(void))
{
  irq_handler = func;
}

void do_irq_handler()
{
  while(int_status != INT_STATUS_RUN);

  irq_handler();

  int_status = INT_STATUS_FIN;
}

void suspend_micon_thread()
{
  while(int_status != INT_STATUS_READY);

  int_status = INT_STATUS_SIGUSR1_SETUP;

  if(pthread_kill(thread_handle, SIGUSR1) != 0)
  {
    printf("[WARN] pthread_kill fail.\n");
  }
}

void resume_micon_thread()
{
  while(int_status != INT_STATUS_FIN);

  if(pthread_kill(thread_handle, SIGUSR2) != 0)
  {
    printf("[WARN] pthread_kill fail.\n");
  }
}

void micon_thread_signal_usr1_handler()
{
  sigset_t signal_set;

  while(int_status != INT_STATUS_SIGUSR1_SETUP);

  printf("usr1 called\n");
  printf("begin sigsuspend\n");

  // mask SIGUSR2 due to SIGUSR2 may catch before sigsuspend
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGUSR2);
  sigprocmask(SIG_SETMASK, &signal_set, NULL);

  int_status = INT_STATUS_RUN;

  // sigsuspend replace empty signal set
  // if SIGUSR2 calls before sigsuspend, immediately resume process
  sigemptyset(&signal_set);
  sigsuspend(&signal_set);
  if(errno != EINTR)
  {
    printf("[WARN] wrong return of sigsuspend(ret = %d).\n", errno);
  }

  int_status = INT_STATUS_READY;

  printf("end sigsuspend\n");
}

void micon_thread_signal_usr2_handler()
{
  printf("usr2 called\n");
}

void micon_thread()
{
  struct sigaction sigusr1, sigusr2;

  // register signal hadlers
  sigusr1.sa_flags   = 0;
  sigusr1.sa_handler = micon_thread_signal_usr1_handler;
  sigemptyset(&sigusr1.sa_mask);
  if(sigaction(SIGUSR1, &sigusr1, NULL) != 0)
  {
    printf("[ERROR] failed to register SIGUSR1.\n");
    exit(1);
  }

  sigusr2.sa_flags   = 0;
  sigusr2.sa_handler = micon_thread_signal_usr2_handler;
  sigemptyset(&sigusr2.sa_mask);
  if(sigaction(SIGUSR2, &sigusr2, NULL) != 0)
  {
    printf("[ERROR] failed to register SIGUSR2.\n");
    exit(1);
  }

  micon_main();
}

int main()
{
  if(pthread_create(&thread_handle, NULL, micon_thread, NULL) != 0)
  {
    printf("[ERROR] failed to create thread.\n");
    exit(1);
  }

  sleep(1);
  for(int i = 0 ; i < 5 ; i++)
  {
    sleep(1);
    suspend_micon_thread();
    do_irq_handler();
    resume_micon_thread();
  }
}

