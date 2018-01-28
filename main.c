#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include "ringbuf.h"

// マイコン側の処理

void register_irq_handler(void (*func)(void));

void uart_rx_handler()
{
  static int     pos   = 0;
  static uint8_t buf[] = "test";

  ringbuf_push(buf[pos]);

//  printf("call uart_rx_handler\n");

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
    }else
    {
 //     printf("empty\n");
    }
  }
}

// 管理側の処理
pthread_t thread_handle;
void      (*irq_handler)(void);
int       int_status = 0;

void register_irq_handler(void (*func)(void))
{
  irq_handler = func;
}

void do_irq_handler()
{
  while(int_status != 1);

  irq_handler();
}

void suspend_micon_thread()
{
  pthread_kill(thread_handle, SIGUSR1);
}

void resume_micon_thread()
{
  pthread_kill(thread_handle, SIGUSR2);
}

void micon_thread_signal_usr1_handler()
{
  sigset_t signal_set;

  while(int_status != 0);

  sigemptyset(&signal_set);
  printf("begin sigsuspend\n");

  int_status = 1;

  sigsuspend(&signal_set);

  int_status = 0;

  printf("end sigsuspend\n");
}

void micon_thread_signal_usr2_handler()
{
  printf("usr2 called\n");
}

void micon_thread()
{
  struct sigaction sigusr1, sigusr2;

  sigusr1.sa_flags   = 0;
  sigusr1.sa_handler = micon_thread_signal_usr1_handler;
  sigemptyset(&sigusr1.sa_mask);
  sigaction(SIGUSR1, &sigusr1, NULL);

  sigusr2.sa_flags   = 0;
  sigusr2.sa_handler = micon_thread_signal_usr2_handler;
  sigemptyset(&sigusr2.sa_mask);
  sigaction(SIGUSR2, &sigusr2, NULL);

  micon_main();
}

int main()
{
  pthread_create(&thread_handle, NULL, micon_thread, NULL);

  for(int i = 0 ; i < 5 ; i++)
  {
    sleep(1);

    suspend_micon_thread();
    do_irq_handler();
    resume_micon_thread();
  }
}

