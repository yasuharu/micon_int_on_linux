#ifdef LINUX_TEST
  #include <stdint.h>
#endif

#define DATA_SIZE (32)
#define ERROR_CHECK

static uint8_t rpos = 0;
static uint8_t wpos = 0;
static uint8_t data[DATA_SIZE];
static uint8_t error_stat = 0;

uint8_t ringbuf_length();

uint8_t ringbuf_pop()
{
  uint8_t ret;

#ifdef ERROR_CHECK
  if(ringbuf_length() == 0)
  {
    error_stat = 1;
  }
#endif

  ret = data[rpos];

  rpos++;
  if(rpos >= DATA_SIZE)
  {
    rpos = 0;
  }

  return ret;
}

void ringbuf_push(uint8_t val)
{
#ifdef ERROR_CHECK
  if(ringbuf_length() == DATA_SIZE - 1)
  {
    error_stat = 1;
  }
#endif

  data[wpos] = val;

  wpos++;
  if(wpos >= DATA_SIZE)
  {
    wpos = 0;
  }
}

uint8_t ringbuf_length()
{
  if(wpos >= rpos)
  {
    return wpos - rpos;
  }else
  {
    return (int8_t)wpos + (int8_t)DATA_SIZE - (int8_t)rpos;
  }
}

uint8_t ringbuf_stat()
{
  return error_stat;
}

