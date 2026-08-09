#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE (1024)

#define PRIO_LOW  (7)
#define PRIO_MED  (5)
#define PRIO_HIGH (3)
#define PRIO_COOP (-1)
#define PRIO_COPP (-2)

K_TIMER_DEFINE(t_low_timer,  NULL, NULL);
K_TIMER_DEFINE(t_med_timer,  NULL, NULL);
K_TIMER_DEFINE(t_high_timer, NULL, NULL);

void t_low_fn(void *p1, void *p2, void *p3)
{
    k_timer_start(&t_low_timer, K_MSEC(300), K_MSEC(300));
    while (1)
    {
        k_timer_status_sync(&t_low_timer);
        LOG_INF("T_LOW running tick=%u", k_uptime_get_32());
    }
}

void t_med_fn(void *p1, void *p2, void *p3)
{
    k_timer_start(&t_med_timer, K_MSEC(200), K_MSEC(200));
    while (1)
    {
        k_timer_status_sync(&t_med_timer);
        LOG_INF("T_MED running tick=%u", k_uptime_get_32());
    }
}

void t_high_fn(void *p1, void *p2, void *p3)
{
    k_timer_start(&t_high_timer, K_MSEC(100), K_MSEC(100));
    while (1)
    {
        k_timer_status_sync(&t_high_timer);
        LOG_INF("T_HIGH running tick=%u", k_uptime_get_32());
    }
}

void t_coop_fn(void *p1, void *p2, void *p3)
{
    LOG_INF("T_COOP Started tick=%u", k_uptime_get_32());
    for (int i = 0; i < 5; i++)
    {
        LOG_INF("T_COOP running step %d/5 tick=%u", i+1, k_uptime_get_32());
        k_busy_wait(40000);
    }
    LOG_INF("T_COOP yield now tick=%u", k_uptime_get_32());

    k_yield();
    LOG_INF("T_COOP Done tick=%u", k_uptime_get_32());
}

void t_copp_fn(void *p1, void *p2, void *p3)
{
    LOG_INF("T_COPP Started tick=%u", k_uptime_get_32());
    k_sleep(K_MSEC(500));
    LOG_INF("T_COPP is resumed now tick=%u", k_uptime_get_32());
}

K_THREAD_DEFINE(thread_low_fn,  STACK_SIZE, t_low_fn,  NULL, NULL, NULL, PRIO_LOW,  0, 0);
K_THREAD_DEFINE(thread_med_fn,  STACK_SIZE, t_med_fn,  NULL, NULL, NULL, PRIO_MED,  0, 0);
K_THREAD_DEFINE(thread_high_fn, STACK_SIZE, t_high_fn, NULL, NULL, NULL, PRIO_HIGH, 0, 0);
K_THREAD_DEFINE(thread_coop_fn, STACK_SIZE, t_coop_fn, NULL, NULL, NULL, PRIO_COOP, 0, 0);
K_THREAD_DEFINE(thread_copp_fn, STACK_SIZE, t_copp_fn, NULL, NULL, NULL, PRIO_COPP, 0, 0);

int main(void)
{
    LOG_INF("Main Function Started tick=%u", k_uptime_get_32());
    return 0;
}

