#include <stdlib.h>
#include <string.h>
#include <nt/ntdll.h>

#define HEAP_CHUNK (64 * 1024)

typedef struct _BLK {
    size_t size;
    int    used;
    struct _BLK *next;
} BLK;

static BLK *g_head = 0;

static BLK *heap_extend(size_t need)
{
    void *mem = 0;
    size_t sz = (need + HEAP_CHUNK - 1) & ~(HEAP_CHUNK - 1);
    if (!NT_SUCCESS(NtAllocateVirtualMemory(&mem, 0, &sz, 0x3000, 0x04)))
        return 0;

    BLK *b = (BLK *)mem;
    b->size = sz - sizeof(BLK);
    b->used = 0;
    b->next = 0;

    if (!g_head) {
        g_head = b;
    } else {
        BLK *p = g_head;
        while (p->next) p = p->next;
        p->next = b;
    }
    return b;
}

void *malloc(size_t n)
{
    n = (n + 15) & ~(size_t)15;
    BLK *p = g_head;
    while (p) {
        if (!p->used && p->size >= n) {
            p->used = 1;
            return (void *)(p + 1);
        }
        p = p->next;
    }
    BLK *b = heap_extend(n);
    if (!b) return 0;
    b->used = 1;
    return (void *)(b + 1);
}

void free(void *ptr)
{
    if (!ptr) return;
    BLK *b = ((BLK *)ptr) - 1;
    b->used = 0;
}

void *calloc(size_t n, size_t sz)
{
    size_t total = n * sz;
    void *p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, size_t n)
{
    if (!ptr) return malloc(n);
    if (!n) { free(ptr); return 0; }
    BLK *b = ((BLK *)ptr) - 1;
    if (b->size >= n) return ptr;
    void *np = malloc(n);
    if (!np) return 0;
    memcpy(np, ptr, b->size);
    free(ptr);
    return np;
}