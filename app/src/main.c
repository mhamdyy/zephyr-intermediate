#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE (1024)
#define PRIO (7)
#define INCREMENTS (1000000)

static struct k_sem sem_finished;
static volatile int counter = 0;
static volatile int counter_no_mutex = 0;

static K_MUTEX_DEFINE(mutex_for_shared_data);

void t_fn(void *p1, void *p2, void *p3)
{
    const char *name = k_thread_name_get(k_current_get());
    int temp = 0;
    LOG_INF("%s Started", name);

    for (int i = 0; i < INCREMENTS; i++)
    {
        /*Shared variable protected with mutex*/
        k_mutex_lock(&mutex_for_shared_data, K_FOREVER);
        temp = counter;
        k_yeild();
        counter = temp + 1;
        k_mutex_unlock(&mutex_for_shared_data);

        /*Unprotected shared variable*/
        temp = counter_no_mutex;
        k_yeild();
        counter_no_mutex = temp + 1;
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

    LOG_INF("Actual Counter final value: %u", counter);
    LOG_INF("Expected Counter final value: %u", 2*INCREMENTS);

    LOG_INF("Actual Counter_no_mutex final value: %u", counter_no_mutex);
    LOG_INF("Expected Counter_no_mutex final value: %u", 2*INCREMENTS);

    if (counter == INCREMENTS * 2)
    {
        LOG_WRN("Counter has no race this run");
    }
    else
    {
        LOG_ERR("Counter Race condition confirmed: lost %d updates", (INCREMENTS * 2) - counter);
    }

    if (counter_no_mutex == INCREMENTS * 2)
    {
        LOG_WRN("Counter_no_mutex No race this run");
    }
    else
    {
        LOG_ERR("Counter_no_mutex Race condition confirmed: lost %d updates", (INCREMENTS * 2) - counter_no_mutex);
    }
    return 0;
}