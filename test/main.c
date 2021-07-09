#include "../src/event_profiling.h"

void *function0(void *args)
{
    int ractor_id = ractor_init_profiling_event_list();
    int id = trace_profiling_event_begin();
    sleep(1);
    trace_profiling_event_end(id);
}

void *function1(void *args)
{
    int ractor_id = ractor_init_profiling_event_list();
    int id = trace_profiling_event_begin();
    sleep(3);
    trace_profiling_event_end(id);
}

int main(void)
{

    init_profiling_event_bucket();

    pthread_t thread_id[8];
    for (int i = 0; i < 4; i++)
        pthread_create(&thread_id[i], NULL, function0, NULL);
    for (int i = 4; i < 8; i++)
        pthread_create(&thread_id[i], NULL, function1, NULL);
    for (int i = 0; i < 8; i++)
        pthread_join(thread_id[i], NULL);

    debug_print_profling_event_bucket();
    serialize_profiling_event_bucket();
    destroy_profiling_event_bucket();
    return 0;
}
