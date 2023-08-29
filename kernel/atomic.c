#include <kernel/atomic.h>


int mutex_lock_test(struct atomic *v)
{
	atomic_inc(v);
	if(v->counter == 1)
		return 1;
	return -1;
}

// 互斥量
int mutex_lock(struct atomic *v)
{
	atomic_inc(v);
	return v->counter;
}

int mutex_unlock(struct atomic *v)
{
	atomic_dec(v);
	return v->counter;
}

int atomic_inc_and_test(struct atomic *v)
{
	unsigned char c;

	__asm__ __volatile__(
		"incl %0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c != 0;
}

int atomic_dec_and_test(struct atomic *v)
{
	unsigned char c;

	__asm__ __volatile__("decl %0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c;
}

int atomic_sub_and_test(struct atomic *v, int i)
{
	unsigned char c;

	__asm__ __volatile__("subl %2,%0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"ir" (i), "m" (v->counter) : "memory");
	return c;
}