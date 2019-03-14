
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

// \see https://akaedu.github.io/book/ch19s05.html
// \see
// https://stackoverflow.com/questions/14251196/how-to-implement-the-sync-fetch-and-sub-atomic-operation-in-assembly-language

typedef struct {
    pthread_t tid;
    int nr_sample;
    int nr_loop;
} worker_t;

#if DEFINE_ATOMICS
#define fastlock_t pthread_mutex_t
#define fastlock_init(lock) pthread_mutex_init(lock, NULL)
#define fastlock_destroy(lock) pthread_mutex_destroy(lock)
#define fastlock_acquire(lock) pthread_mutex_lock(lock)
#define fastlock_release(lock) pthread_mutex_unlock(lock)
#else
typedef struct {
    sem_t sem;
    volatile int cnt;
} fastlock_t;

static inline void fastlock_init(fastlock_t *lock)
{
    sem_init(&lock->sem, 0, 0);
    lock->cnt = 0;
}

static inline void fastlock_destroy(fastlock_t *lock)
{
    sem_destroy(&lock->sem);
}

// \see https://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
// type __sync_add_and_fetch(type *ptr, type value, ...)
// { *ptr op= value; return *ptr; }
//
// \see https://en.wikipedia.org/wiki/Fetch-and-add#x86_implementation
// inline unsigned int __sync_add_and_fetch(volatile unsigned int *p,
//                                          unsigned int incr)
// {
//     // lock xaddl op0, op1 := atomic { t = op1; op1 = op0 + op1; op0 = t; }
//     __asm__ __volatile("lock xaddl %0, %1\n\taddl %0, %0" : "+r"(incr),
//     "+m"(*p) : : "memory");
//
//     return incr;
// }
//
// \note gcc -S main.c -o main.s
// moveq -8(%rbp), %rax
// leaq 32(%rax), %rdx
// movl $1, %eax
// lock xaddl %eax, (%rdx)
// addl $1, $eax
// cmpl $1, $eax
// jle .L5 (nop && leave && ret)
//
// \see
// https://web.cecs.pdx.edu/~kimchris/cs201/slides/10%20-%20x86%20Basics,%20Part%202.pdf
static inline void fastlock_acquire(fastlock_t *lock)
{
    if (__sync_add_and_fetch(&lock->cnt, 1) > 1)
        sem_wait(&lock->sem);
}

// \note gcc -S main.c -o main.s
// movq -8(%rbp), %rax
// leaq 32(%rax), %rdx
// movl $1, %eax
// negl %eax
// movl %eax, %ecx
// movl %ecx, %eax
// lock xaddl %eax, ($rdx)
// addl %ecx, %eax
// testl %eax, %eax
// jle .L8 (nop && leave && ret)
//
// \see https://stackoverflow.com/questions/13064809/the-point-of-test-eax-eax
static inline void fastlock_release(fastlock_t *lock)
{
    if (__sync_sub_and_fetch(&lock->cnt, 1) > 0)
        sem_post(&lock->sem);
}
#endif

static fastlock_t slock;

static inline void busyloop(int nr_loop)
{
    for (int i = 0; i < nr_loop; i++) {
        __asm__("nop");
    }
}

void *worker_main(void *argp)
{
    worker_t *worker_args = (worker_t *)argp;
    int nr_sample = worker_args->nr_sample;
    int nr_loop = worker_args->nr_loop;

    for (int i = 0; i < nr_sample; i++) {
        fastlock_acquire(&slock);
        busyloop(nr_loop);
        fastlock_release(&slock);
    }
}

#define NR_CMDLINE_ARGS (3 + 1)

void print_usage(const char *prgname)
{
    printf("Usage: %s <nr_thread> <nr_loop> <nr_sample>\n", prgname);
}

int main(int argc, char *argv[])
{
    if (argc != NR_CMDLINE_ARGS) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    int nr_thread = atol(argv[1]);
    int nr_loop = atol(argv[2]);
    int nr_sample = atol(argv[3]);

    fastlock_init(&slock);
    worker_t worker[nr_thread];

    for (int i = 0; i < nr_thread; i++) {
        worker[i].nr_loop = nr_loop;
        worker[i].nr_sample = nr_sample;
    }

    // n-1 workers
    for (int i = 1; i < nr_thread; i++)
        pthread_create(&worker[i].tid, NULL, worker_main, &worker[i]);

    // Also as a worker
    worker_main(&worker[0]);

    for (int i = 1; i < nr_thread; i++)
        pthread_join(worker[i].tid, NULL);

    fastlock_destroy(&slock);

    return 0;
}
