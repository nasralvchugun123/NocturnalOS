#ifndef NT_ATOMIC_H
#define NT_ATOMIC_H

#include <stdint.h>

static inline void AtomicMemoryBarrier(void) { __asm__ volatile ("mfence" ::: "memory"); }
static inline void AtomicLoadBarrier(void)   { __asm__ volatile ("lfence" ::: "memory"); }
static inline void AtomicStoreBarrier(void)  { __asm__ volatile ("sfence" ::: "memory"); }

static inline uint32_t AtomicExchange32(volatile uint32_t *p, uint32_t v)
{
    uint32_t old;
    __asm__ volatile ("xchgl %0, %1"
                      : "=r"(old), "+m"(*p)
                      : "0"(v)
                      : "memory");
    return old;
}


static inline uint32_t AtomicFetchAdd32(volatile uint32_t *p, uint32_t value)
{
    uint32_t old = value;
    __asm__ volatile ("lock xaddl %0, %1"
                      : "+r"(old), "+m"(*p)
                      :
                      : "memory", "cc");
    return old;
}

static inline int64_t AtomicLoadAcquire64(volatile int64_t *p)
{
    int64_t value = *p;
    AtomicLoadBarrier();
    return value;
}

static inline uint64_t AtomicExchange64(volatile uint64_t *p, uint64_t v)
{
    uint64_t old;
    __asm__ volatile ("xchgq %0, %1"
                      : "=r"(old), "+m"(*p)
                      : "0"(v)
                      : "memory");
    return old;
}

static inline int32_t AtomicCompareExchange32(volatile int32_t *p,
                                              int32_t expected, int32_t desired)
{
    int32_t old;
    __asm__ volatile ("lock cmpxchgl %2, %1"
                      : "=a"(old), "+m"(*p)
                      : "r"(desired), "0"(expected)
                      : "memory");
    return old;
}

static inline int64_t AtomicCompareExchange64(volatile int64_t *p,
                                              int64_t expected, int64_t desired)
{
    int64_t old;
    __asm__ volatile ("lock cmpxchgq %2, %1"
                      : "=a"(old), "+m"(*p)
                      : "r"(desired), "0"(expected)
                      : "memory");
    return old;
}

static inline int64_t AtomicIncrement64(volatile int64_t *p)
{
    int64_t v;
    __asm__ volatile ("lock xaddq %0, %1"
                      : "=r"(v), "+m"(*p)
                      : "0"((int64_t)1)
                      : "memory");
    return v + 1;
}

static inline int64_t AtomicDecrement64(volatile int64_t *p)
{
    int64_t v;
    __asm__ volatile ("lock xaddq %0, %1"
                      : "=r"(v), "+m"(*p)
                      : "0"((int64_t)-1)
                      : "memory");
    return v - 1;
}

static inline uint32_t AtomicLoadAcquire32(volatile uint32_t *p)
{
    uint32_t v = *p;
    AtomicLoadBarrier();
    return v;
}

static inline void AtomicStoreRelease32(volatile uint32_t *p, uint32_t v)
{
    AtomicStoreBarrier();
    *p = v;
}

#endif