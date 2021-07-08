#include "event_profiling.h"

/* Global bucket */
profiling_event_bucket_t *rb_profiling_event_bucket;

/* Internal functions */

#define refute_null(var, reason)                                               \
    do                                                                         \
    {                                                                          \
        if (var == NULL)                                                       \
        {                                                                      \
            fprintf(stderr, (reason));                                         \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

static inline profiling_event_list_t *init_profiling_event_list(int ractor_id)
{
    profiling_event_list_t *list =
        (profiling_event_list_t *)malloc(sizeof(profiling_event_list_t));
    refute_null(list, "Failed to allocate memory for profiling event list\n");

    profiling_event_t *event = (profiling_event_t *)malloc(
        sizeof(profiling_event_t) * EVENT_PROFILING_RACTOR_MAX_EVENTS);
    refute_null(
        event, "Failed to allocate internal memory for profiling event list\n");

    list->ractor_id = ractor_id;
    list->tail = 0;
    list->event_id = 0;
    list->event = event;

    return list;
}

static inline profiling_event_t *
get_a_profiling_event_slot(const int ractor_id);

static inline void serialize_profiling_event(const profiling_event_t *event);

static inline void
serialize_profiling_event_list(const profiling_event_list_t *list);

static inline void destory_profiling_event_list(profiling_event_list_t *list);

/* Internal debugging facilities */
#ifdef DEBUG_EVENT_PROFILING
static inline void debug_print_profling_event(const profiling_event_t *event) {}
static inline void debug_print_profling_event_list(const profiling_event_list_t *list)
{
}
static inline void debug_print_profling_event_bucket() {}
#else
static inline void debug_print_profling_event(const profiling_event_t *event) {}
static inline void debug_print_profling_event_list(const profiling_event_list_t *list)
{
}
static inline void debug_print_profling_event_bucket() {}
#endif

/* Public functions */
profiling_event_bucket_t *init_profiling_event_bucket()
{
    profiling_event_bucket_t *bucket =
        (profiling_event_bucket_t *)malloc(sizeof(profiling_event_bucket_t));
    refute_null(bucket,
                "Failed to allocate memory for rb_profiling_event_bucket.\n");

    profiling_event_list_t *first_list = init_profiling_event_list(0);
    refute_null(first_list,
                "Failed to allocate memory for first profiling event list.\n");

    pthread_mutex_init(&(bucket->bucket_lock), NULL);
    bucket->ractors = 1;
    bucket->ractor_profiling_event_list[0] = first_list;

    rb_profiling_event_bucket = bucket;
    return bucket;
}

void ractor_init_profiling_event_list() {
    int ractor_id = -1;
    pthread_mutex_lock(&(rb_profiling_event_bucket->bucket_lock));
    ractor_id = rb_profiling_event_bucket->ractors++;
    pthread_mutex_unlock(&(rb_profiling_event_bucket->bucket_lock));

    profiling_event_list_t *list = init_profiling_event_list(ractor_id);
    refute_null(list, "Failed to allocate memory for profiling event list\n");

    rb_profiling_event_bucket->ractor_profiling_event_list[ractor_id] = list;
}

void destory_profiling_event_bucket(profiling_event_bucket_t *bucket);

int tet_trace_profiling_event(const char *file, const char *func,
                              const int line, const int event_id,
                              const profiling_event_phase_t phase);
