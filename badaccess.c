#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>

#define MAP_BASE 0xc0000000u
#define MAP_END  0xd0000000u
#define MAP_SIZE 0x10000000u
#define MAP_MASK (MAP_SIZE - 1u)

struct xorshift32_state {
  uint32_t a;
};

/* The state word must be initialized to non-zero */
static inline uint32_t
xorshift32(struct xorshift32_state *state)
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = state->a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return state->a = x;
}

static jmp_buf g_env;
static pthread_mutex_t g_mtx = PTHREAD_MUTEX_INITIALIZER;
volatile static siginfo_t g_siginfo;
volatile static char g_bad_val;
volatile static int num_sigs = 0;
static void
handler(int const signo, siginfo_t *const info, void *const context)
{
    ++num_sigs;
    g_siginfo = *info;
    siglongjmp(g_env, 1);
}

static int
access_bad_addr(void const*const addr)
{
    volatile int counter = 0;

    (void)pthread_mutex_lock(&g_mtx);

    int status = sigsetjmp(g_env, 1);

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;

    if (status == 0) {
        status = sigaction(SIGSEGV, &sa, NULL);
        if (status == -1) {
            (void)pthread_mutex_unlock(&g_mtx);
            return -1;
        }
        g_bad_val = *(char const*)addr;
    }
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &sa, NULL);

    (void)pthread_mutex_unlock(&g_mtx);
    return counter;
}

typedef struct {
    pthread_t thr;
    int id;
    pthread_barrier_t *barrierp;
} thread_context; 

void *
thread_work(void *const arg)
{
    thread_context *const ctxt = arg;
    (void)pthread_barrier_wait(ctxt->barrierp);

    struct xorshift32_state rng = { .a = time(NULL) };
    for (int i = 0; i < (ctxt->id + 1) * 10; ++i) {
        (void)xorshift32(&rng);
    }

    for (int i = 0; i < 1000; ++i) {
        void *const addr = (void *)0xc0000000 + (xorshift32(&rng) & MAP_MASK);
        //printf("thread %d accessing %lx\n", ctxt->id, (uintptr_t)addr);
        //usleep(5000);
        access_bad_addr(addr);
    }

    return NULL;
}

#define NUM_THREADS 5

int
main(int argc, char **argv)
{
    access_bad_addr((void *)0x442);

    thread_context *arr = malloc(sizeof(*arr) * NUM_THREADS);


    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; ++i) {
        arr[i].barrierp = &barrier;
        arr[i].id = i;
        pthread_create(&arr[i].thr, NULL, thread_work, &arr[i]);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(arr[i].thr, NULL);
    }

    printf("handled %d SIGSEGVs\n", num_sigs);

    return 0;
}
