#include "../src/event_profiling.h"

void *function0(void *args)
{
    sleep(3);
    ractor_init_profiling_event_list();
    sleep(3);
}

int main(void)
{
    rb_profiling_event_bucket = NULL;
    init_profiling_event_bucket();
    printf("%d\n", rb_profiling_event_bucket);
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, function0, NULL);
    pthread_join(thread_id, NULL);
    return 0;
}
