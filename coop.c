#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>

struct coop {
    struct coop *next;
    int id;
    void *return_addr;
    void *(*start_routine)(void*);
};

static int coop_id = 1;
static struct coop *run_list = NULL;
static struct coop *current = NULL;
static ptrdiff_t ra_offset;

void
coop_sleep (int secs)
{
}

void
coop_yield (void)
{
    struct coop *next;
    current->return_addr = __builtin_return_address(0);

    if (current->next == NULL) {
        next = run_list;
    } else {
        next = current->next;
    }

    current = next;
    void *bfa = __builtin_frame_address(0); 
    *(void **)(bfa + ra_offset) = next->return_addr;
}


void
init_ra_offset (void)
{
    void **fp;
    void *my_ra = __builtin_return_address(0);

    fp = __builtin_frame_address(0);
    while (*fp != my_ra) {
        fp++;
    }

    ra_offset = (void *)fp - __builtin_frame_address(0);
}

struct coop *
coop_create (void *(*start_routine)(void*), void *arg)
{
    struct coop *coop;
    void *ret_ptr;

    current->return_addr = __builtin_return_address(0);

    coop = malloc(sizeof(struct coop));
    if (coop == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    coop->next = run_list;
    run_list = coop;
    coop->id = coop_id++;
    coop->start_routine = start_routine;
   
    current = coop;
    ret_ptr = coop->start_routine(arg);
}


int
coop_start (void *(*start_routine)(void*), void *arg)
{
    struct coop *coop;
    void *ret_ptr;

    init_ra_offset();
    
    coop = malloc(sizeof(struct coop));
    if (coop == NULL) {
        errno = ENOMEM;
        return -1;
    }

    coop->next = NULL;
    coop->id = coop_id++;
    coop->start_routine = start_routine;

    run_list = coop;
    current = coop;
    
    ret_ptr = coop->start_routine(arg);
    
    return 0;
}


void *
coop_first_yield (void)
{
    coop_yield();
}

void *
coop_second_yield (void)
{
    coop_yield();
}

void *
coop_third_yield (void)
{
    coop_yield();
}

void *
coop_third (void *arg)
{
    int count = 0;
    
    puts("coop_third() entry");
    while (1) {
        printf("coop_third() count=%d\n", count);
        count--;
        sleep(1);
        coop_third_yield();
    }
    return NULL;
}

void *
coop_second (void *arg)
{
    int count = 0;
    
    puts("coop_second() entry");
    while (1) {
        printf("coop_second() count=%d\n", count);
        count--;
        sleep(1);
        coop_second_yield();
    }
    return NULL;
}

void *
coop_first (void *arg)
{
    int count = 0;
    
    puts("coop_first() entry");
    coop_create(coop_second, NULL);
    coop_create(coop_third, NULL);
    while (1) {
        printf("coop_first() count=%d\n", count);
        count++;
        sleep(1);
        coop_first_yield();
    }
    return NULL;
}


int
main (int argc, char **argv)
{
    int exit_code;
    exit_code = coop_start(coop_first, NULL);

    return exit_code;
}
