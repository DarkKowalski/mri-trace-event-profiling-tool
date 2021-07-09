#include "event_profiling.h"

/* Global bucket */
profiling_event_bucket_t *rb_profiling_event_bucket;

/* Phase strings */
static const char profiling_event_phase_str[] = {'B', 'E'};

/* Internal functions */

#define refute_null(var, reason)                                               \
    do                                                                         \
    {                                                                          \
        if ((var) == NULL)                                                     \
        {                                                                      \
            fprintf(stderr, (reason));                                         \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define refute_greater_or_equal(var, compare, reason)                          \
    do                                                                         \
    {                                                                          \
        if ((var) >= (compare))                                                \
        {                                                                      \
            fprintf(stderr, (reason));                                         \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

static inline time_t microsecond_timestamp()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    time_t us = t.tv_sec * 1E6 + t.tv_nsec / 1E3;

    return us;
}

static inline profiling_event_list_t *
init_profiling_event_list(const int ractor_id)
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

static inline profiling_event_t *get_a_profiling_event_slot(const int ractor_id)
{
    profiling_event_list_t *list =
        rb_profiling_event_bucket->ractor_profiling_event_list[ractor_id - 1];

    int slot_index = list->tail++;
    refute_greater_or_equal(slot_index, EVENT_PROFILING_RACTOR_MAX_EVENTS,
                            "To many events.\n");

    profiling_event_t *event = &(list->event[slot_index]);
    event->ractor = ractor_id;

    return event;
}

static inline int get_a_new_event_id(const int ractor_id)
{
    profiling_event_list_t *list =
        rb_profiling_event_bucket->ractor_profiling_event_list[ractor_id - 1];
    return list->event_id++;
}

static inline int serialize_profiling_event(const profiling_event_t *event,
                                            char *buffer, const int offset)
{
    char *event_buffer = buffer + offset;

    return sprintf(event_buffer,
                   "{\"name\": \"%s:%s(%d)\",\n"
                   "\"ph\":\"%c\",\n"
                   "\"pid\":\"%i\",\n"
                   "\"tid\":\"%i\",\n"
                   "\"ts\":\"%ld\",\n"
                   "\"args\": {\"line\": \"%d\", \"ractor\":\"%d\"}},\n",
                   event->file, event->function, event->id,
                   profiling_event_phase_str[event->phase], event->pid,
                   event->tid, event->timestamp, event->line, event->ractor);
}

static inline int
serialize_profiling_event_list(const profiling_event_list_t *list, char *buffer,
                               const int offset)
{
    int events = list->tail;
    int list_offset = offset;
    for (int i = 0; i < events; i++)
    {
        list_offset +=
            serialize_profiling_event(&(list->event[i]), buffer, list_offset);
    }
    return list_offset;
}

static inline void destroy_profiling_event_list(profiling_event_list_t *list)
{
    refute_null(list, "Cannot destroy null profiling event list.\n");
    refute_null(list->event, "Cannot destroy bad profiling event list.\n");

    free(list->event);
    free(list);
}

/* Internal debugging facilities */
#if DEBUG_EVENT_PROFILING
static inline void debug_print_profling_event(const profiling_event_t *event)
{
    printf("file = %s\n"
           "function = %s\n"
           "line = %d\n"
           "ractor = %d\n"
           "id = %d\n"
           "pid = %d\n"
           "tid = %d\n"
           "phase = %c\n"
           "timestamp = %ld\n\n",
           event->file, event->function, event->line, event->ractor, event->id,
           event->pid, event->tid, profiling_event_phase_str[event->phase], event->timestamp);
}

static inline void
debug_print_profling_event_list(const profiling_event_list_t *list)
{
    int events = list->tail;
    for (int i = 0; i < events; i++)
    {
        debug_print_profling_event(&(list->event[i]));
    }
}

void debug_print_profling_event_bucket()
{
    pthread_mutex_lock(&(rb_profiling_event_bucket->bucket_lock));
    int ractors = rb_profiling_event_bucket->ractors;
    for (int i = 0; i < ractors; i++)
    {
        debug_print_profling_event_list(
            rb_profiling_event_bucket->ractor_profiling_event_list[i]);
    }
    pthread_mutex_unlock(&(rb_profiling_event_bucket->bucket_lock));
}
#else
void debug_print_profling_event_bucket() {}
#endif

/* Public functions */
profiling_event_bucket_t *init_profiling_event_bucket()
{
    profiling_event_bucket_t *bucket =
        (profiling_event_bucket_t *)malloc(sizeof(profiling_event_bucket_t));
    refute_null(bucket,
                "Failed to allocate memory for rb_profiling_event_bucket.\n");

    profiling_event_list_t *first_list = init_profiling_event_list(1);
    refute_null(first_list,
                "Failed to allocate memory for first profiling event list.\n");

    pthread_mutex_init(&(bucket->bucket_lock), NULL);
    bucket->ractors = 1;
    bucket->ractor_profiling_event_list[0] = first_list;

    rb_profiling_event_bucket = bucket;
    return bucket;
}

int ractor_init_profiling_event_list()
{
    pthread_mutex_lock(&(rb_profiling_event_bucket->bucket_lock));
    int ractor_id = ++(rb_profiling_event_bucket->ractors);
    pthread_mutex_unlock(&(rb_profiling_event_bucket->bucket_lock));
    refute_greater_or_equal(
        ractor_id, EVENT_PROFILING_MAX_RACTORS + 1,
        "Too many Ractors.\n"); /* ractor_id starts from 1 */

    profiling_event_list_t *list = init_profiling_event_list(ractor_id);
    refute_null(list, "Failed to allocate memory for profiling event list\n");

    rb_profiling_event_bucket->ractor_profiling_event_list[ractor_id - 1] =
        list;

    return ractor_id;
}

void destroy_profiling_event_bucket()
{
    pthread_mutex_lock(&(rb_profiling_event_bucket->bucket_lock));
    refute_null(rb_profiling_event_bucket, "Cannot destroy null bucket.\n");

    int ractors = rb_profiling_event_bucket->ractors;
    for (int i = 0; i < ractors; i++)
    {
        destroy_profiling_event_list(
            rb_profiling_event_bucket->ractor_profiling_event_list[i]);
    }
    pthread_mutex_unlock(&(rb_profiling_event_bucket->bucket_lock));

    free(rb_profiling_event_bucket);
}

int trace_profiling_event(const char *file, const char *func, const int line,
                          const int                     event_id,
                          const profiling_event_phase_t phase)
{
    // TODO: get ractor_id;
    int ractor_id = 1;

    profiling_event_t *event = get_a_profiling_event_slot(ractor_id);
    int                id = (event_id == NEW_PROFILING_EVENT_ID)
                                ? get_a_new_event_id(ractor_id)
                                : event_id;

    event->file = (char *)file;
    event->function = (char *)func;
    event->line = line;
    event->id = id;

    event->phase = phase;

    event->pid = getpid();
    event->tid = gettid();

    event->timestamp = microsecond_timestamp();

    return id;
}

void serialize_profiling_event_bucket()
{
    char *bucket_buffer =
        (char *)malloc(sizeof(char) * EVENT_PROFILING_BUCKET_BUFFER);
    refute_null(bucket_buffer,
                "Failed to allocate memory for event bucket buffer.\n");

    int offset = sprintf(bucket_buffer, "[");

    pthread_mutex_lock(&(rb_profiling_event_bucket->bucket_lock));
    int ractors = rb_profiling_event_bucket->ractors;
    for (int i = 0; i < ractors; i++)
    {
        profiling_event_list_t *list =
            rb_profiling_event_bucket->ractor_profiling_event_list[i];
        offset = serialize_profiling_event_list(list, bucket_buffer, offset);
    }
    pthread_mutex_unlock(&(rb_profiling_event_bucket->bucket_lock));
    sprintf(bucket_buffer + offset - 2, "]\n"); /* Remove the last `,` */

    FILE *stream = fopen(EVENT_PROFILING_OUTFILE, "w");
    refute_null(stream, "Failed to write file.\n");
    fputs(bucket_buffer, stream);
    fclose(stream);

    free(bucket_buffer);
}
