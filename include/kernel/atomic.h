#ifndef _ATOMIC_H
#define _ATOMIC_H

struct atomic
{
    volatile int counter;
};

#define atomic_read(v)		((v)->counter)

#define atomic_add(v,i) __asm__("addl %1,%0":"=m"((v)->counter):"ir"(i),"m"((v)->counter))
#define atomic_sub(v,i) __asm__("subl %1,%0":"=m"((v)->counter):"ir"(i),"m"((v)->counter))
#define atomic_inc(v) 	__asm__("incl %0":"=m"((v)->counter):"m"((v)->counter))
#define atomic_dec(v)	__asm__("decl %0":"=m"((v)->counter):"m"((v)->counter))



// 多核处理器使用 LOCK TODO
#define LOCK "lock ; "


extern  int mutex_lock_test(struct atomic *v);
extern  int mutex_lock(struct atomic *v);		// 互斥量
extern  int mutex_unlock(struct atomic *v);
extern  int atomic_dec_and_test(struct atomic *v);
extern  int atomic_inc_and_test(struct atomic *v);
extern  int atomic_sub_and_test(struct atomic *v, int i);


#endif