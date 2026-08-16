#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE (1024)
#define PRIO (7)
#define INCREMENTS (1000000)

static struct k_sem sem_finished;
static volatile int counter = 0;

void t_fn(void *p1, void *p2, void *p3)
{
    const char *name = k_thread_name_get(k_current_get());
    LOG_INF("%s Started", name);

    for (int i = 0; i < INCREMENTS; i++)
    {
        counter ++;
    }

    LOG_INF("%s Finished", name);
    k_sem_give(&sem_finished);
}


K_THREAD_DEFINE(thread_a,  STACK_SIZE, t_fn,  NULL, NULL, NULL, PRIO,  0, 0);
K_THREAD_DEFINE(thread_b,  STACK_SIZE, t_fn,  NULL, NULL, NULL, PRIO,  0, 0);


int main(void)
{
    LOG_INF("Main Function Started");
    k_sem_init(&sem_finished, 0, 1);

    LOG_INF("Expected final value: %d", INCREMENTS * 2);

    k_sem_take(&sem_finished, K_FOREVER);
    k_sem_take(&sem_finished, K_FOREVER);

    LOG_INF("Actual  final value: %u", counter);

    if (counter == INCREMENTS * 2)
    {
        LOG_WRN("No race this run");
    }
    else
    {
        LOG_ERR("Race condition confirmed: lost %d updates", (INCREMENTS * 2) - counter);
    }
    return 0;
}

