/*
 * randnum.c -- generate (good) randum numbers.
 *
 * Copyright (C) 2001,2002 by Theodore Ts'o
 * 
 * This file may be distributed under the terms of the GNU Public
 * License.
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "pwgen.h"

#ifdef HAVE_DRAND48
extern double drand48(void);
#endif

static int get_random_fd(void);

/* Borrowed/adapted from e2fsprogs's UUID generation code */
static int get_random_fd()
{
	struct timeval	tv;
	static int	fd = -2;
	int		i;

	if (fd == -2) {
		gettimeofday(&tv, 0);
		fd = open("/dev/urandom", O_RDONLY);
		if (fd == -1)
			fd = open("/dev/random", O_RDONLY | O_NONBLOCK);
#ifdef HAVE_DRAND48
		srand48((tv.tv_sec<<9) ^ (getpgrp()<<15) ^
			(getpid()) ^ (tv.tv_usec>>11));
#else
		srandom((getpid() << 16) ^ (getpgrp() << 8) ^ getuid() 
		      ^ tv.tv_sec ^ tv.tv_usec);
#endif
	}
	/* Crank the random number generator a few times */
	gettimeofday(&tv, 0);
	for (i = (tv.tv_sec ^ tv.tv_usec) & 0x1F; i > 0; i--)
#ifdef HAVE_DRAND48
		drand48();
#else
		random();
#endif
	return fd;
}


static long double probability;
static int num_rand_steps;
static int rand_path[65536];

extern void new_password(void)
{
	probability = 1.0;
	num_rand_steps = 0;
}

extern long double get_probability(void)
{
	return probability;
}

extern int get_num_rand_steps(void)
{
	return num_rand_steps;
}

extern int get_rand_step(int num)
{
	assert(num >= 0);
	assert(num < num_rand_steps);
	return rand_path[num];
}

/*
 * Generate a random number n, where 0 <= n < max_num, using
 * /dev/urandom if possible.
 */
int pw_random_number(max_num)
	int max_num;
{
	int i, fd = get_random_fd();
	int lose_counter = 0, nbytes=4;
	unsigned int rand_num;
	char *cp = (char *) &rand_num;
	int ret;

	probability *= 1.0 / (max_num+1);

	if (fd >= 0) {
		while (nbytes > 0) {
			i = read(fd, cp, nbytes);
			if ((i < 0) &&
			    ((errno == EINTR) || (errno == EAGAIN)))
				continue;
			if (i <= 0) {
				if (lose_counter++ == 8)
					break;
				continue;
			}
			nbytes -= i;
			cp += i;
			lose_counter = 0;
		}
	}
	if (nbytes == 0) {
		ret = (rand_num % max_num);
		rand_path[num_rand_steps++] = ret;
		return ret;
	}

	/* OK, we weren't able to use /dev/random, fall back to rand/rand48 */

#ifdef HAVE_DRAND48
	ret = ((int) ((drand48() * max_num)));
#else
	ret = ((int) (random() / ((float) RAND_MAX) * max_num));
#endif
	rand_path[num_rand_steps++] = ret;
	return ret;
}
