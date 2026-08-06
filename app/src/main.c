#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024

#define PRIO_LOW  7
#define PRIO_MED  5
#define PRIO_HIGH 3

void t_low_fn(void *p1, void *p2, void *p3)
{
    while (1)
    {
        LOG_INF("T_LOW running tick=%u", k_uptime_get_32());
        k_msleep(300);
    }
}

void t_med_fn(void *p1, void *p2, void *p3)
{
    while (1)
    {
        LOG_INF("T_MED running tick=%u", k_uptime_get_32());
        k_msleep(200);
    }
}

void t_high_fn(void *p1, void *p2, void *p3)
{
    while (1)
    {
        LOG_INF("T_HIGH running tick=%u", k_uptime_get_32());
        k_msleep(100);
    }
}

K_THREAD_DEFINE(thread_low_fn,  STACK_SIZE, t_low_fn,  NULL, NULL, NULL, PRIO_LOW,  0, 0);
K_THREAD_DEFINE(thread_med_fn,  STACK_SIZE, t_med_fn,  NULL, NULL, NULL, PRIO_MED,  0, 0);
K_THREAD_DEFINE(thread_high_fn, STACK_SIZE, t_high_fn, NULL, NULL, NULL, PRIO_HIGH, 0, 0);

int main(void)
{
    LOG_INF("Main Function Started tick=%u", k_uptime_get_32());
    return 0;
}

