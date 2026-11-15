#ifndef HH_RUNTIME_TASK_H
#define HH_RUNTIME_TASK_H

#include <stdbool.h>

/* RA task hooks, not a product control API. */
void *hh_runtime_state_task_attach(void);
void hh_runtime_state_task_complete(void *token, bool success);

#endif
