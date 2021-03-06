
/* advfio.c - file i/o routines for the adventure compiler */
/*
	Copyright (c) 1986, by David Michael Betz
	All rights reserved
*/

#define BSIZE	8192

/* global variables */
long ad_foff;

/* external routines */
extern long lseek();

/* local variables */
static char buf[BSIZE];
static int boff;
static int fd;

ad_create(name)
  char *name;
{
    /* create the file */
    if ((fd = creat(name,0666)) < 0)
	fail("can't create output file");
	
    /* initialize the buffer and file offset */
    ad_foff = 0L;
    boff = 0;
}

ad_close()
{
    ad_flush();
    close(fd);
}

ad_putc(ch)
  int ch;
{
    buf[boff++] = ch; ad_foff++;
    if (boff >= BSIZE)
	ad_flush();
}

ad_seek(pos)
  long pos;
{
    ad_flush();
    if (lseek(fd,pos,0) != pos)
	fail("error positioning output file");
    ad_foff = pos;
}

ad_flush()
{
    if (boff) {
	if (write(fd,buf,boff) != boff)
	    fail("error writing to output file");
	boff = 0;
    }
}
