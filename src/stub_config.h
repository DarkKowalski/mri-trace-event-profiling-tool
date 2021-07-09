#ifndef STUB_CONFIG_H

#define USE_EVENT_PROFILING   1
#define DEBUG_EVENT_PROFILING 1

#define EVENT_PROFILING_RACTOR_MAX_EVENTS (64 * 1024)
#define EVENT_PROFILING_MAX_RACTORS       (16)

#define EVENT_PROFILING_BUFFER_PADDING (64)
#define EVENT_PROFILING_EVENT_BUFFER (256)
#define EVENT_PROFILING_LIST_BUFFER (EVENT_PROFILING_EVENT_BUFFER * EVENT_PROFILING_RACTOR_MAX_EVENTS + EVENT_PROFILING_BUFFER_PADDING)
#define EVENT_PROFILING_BUCKET_BUFFER (EVENT_PROFILING_LIST_BUFFER * EVENT_PROFILING_MAX_RACTORS + EVENT_PROFILING_BUFFER_PADDING)

#define EVENT_PROFILING_OUTFILE "event_profiling.json"

#endif
