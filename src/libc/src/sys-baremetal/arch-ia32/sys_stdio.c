#include <stdio.h>
#include <stdint.h>

#if 0
static inline void
outb(unsigned int port, uint8_t value )
{
	__asm__ __volatile__ ("outb %1, %%dx" : :"d"(port), "al"(value));
}

static inline uint8_t
inb(unsigned int port)
{
	uint8_t tmp;
	__asm__ __volatile__ ("inb %%dx, %0" :"=al"(tmp) :"d"(port));
	return tmp;
}
#endif

/* Taken from glibc */
static __inline unsigned char inb (unsigned short int port)
{
  unsigned char _v;

  __asm__ __volatile__ ("inb %w1,%0":"=a" (_v):"Nd" (port));
  return _v;
}

static __inline void outb (unsigned short int port, unsigned char value)
{
  __asm__ __volatile__ ("outb %b0,%w1": :"a" (value), "Nd" (port));
}


#define COMPORT 0x3f8

static int
ser_out(int c)
{
	/* Do file output! */
	while (!(inb(COMPORT+5) & 0x60));
	outb(COMPORT,c);
	if (c == '\n')
		ser_out('\r');
	return 0;
}

static size_t
l4kdb_write(void *data, long int position, size_t count, void *handle /*unused*/)
{
	size_t i;
	char *real_data = data;
	for (i = 0; i < count; i++)
		ser_out(real_data[i]);
	return count;
}

struct __file __stdin = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	_IONBF,
	NULL,
	0,
	0
};


struct __file __stdout = {
	NULL,
	NULL,
	l4kdb_write,
	NULL,
	NULL,
	_IONBF,
	NULL,
	0,
	0
};


struct __file __stderr = {
	NULL,
	NULL,
	l4kdb_write,
	NULL,
	NULL,
	_IONBF,
	NULL,
	0,
	0
};

FILE *stdin = &__stdin;
FILE *stdout = &__stdout;
FILE *stderr = &__stderr;
