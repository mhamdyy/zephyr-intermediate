#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/task_wdt/task_wdt.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE              (2048)
#define PRIO                    (7)
#define PIPELINE_QUEUE_DEPTH    (6)
#define SENSOR_DATA_COUNT       (20)

typedef struct _sensor_data {
    int32_t temperature_mc;
    uint32_t timestamp_ms;
    uint8_t seq;
}sensor_data;

K_MSGQ_DEFINE(pipeline_queue, sizeof(sensor_data), PIPELINE_QUEUE_DEPTH, 4);

bool sensor_thread_finished = false;
bool logger_thread_finished = false;

void sensor_wgt_miss()
{
    LOG_ERR("Sensor thread watchdog expired");
}

void logger_wgt_miss()
{
    LOG_ERR("Logger thread watchdog expired");
}

void sensor_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    int wdt_sensor = task_wdt_add(2000, sensor_wgt_miss, (void*) k_current_get());

    k_thread_name_set(k_current_get(), "sensor");

    sensor_data data;

    for (int i =0; i< SENSOR_DATA_COUNT; i++)
    {
        data.temperature_mc = 24000 + (i * 350);
        data.timestamp_ms = k_uptime_get_32();
        data.seq = (uint8_t)i;

        LOG_INF("[SENSOR] puts seq=%u temp=%d mC", data.seq, data.temperature_mc);

        int ret = k_msgq_put(&pipeline_queue, &data, K_MSEC(200));

        if (ret != 0)
        {
            LOG_WRN("[SENSOR] put failed ret=%d", ret);
        }
        k_msleep(50);
        task_wdt_feed(wdt_sensor);
    }

    task_wdt_delete(wdt_sensor);

    sensor_thread_finished = true;
}

void logger_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "logger");

    int wdt_logger = task_wdt_add(2000, logger_wgt_miss, (void*) k_current_get());

    for (int i = 0; i < SENSOR_DATA_COUNT; i++)
    {
        sensor_data msg;

        int ret = k_msgq_get(&pipeline_queue, &msg, K_MSEC(300));

        if (ret != 0)
        {
            LOG_WRN("[LOGGER-MSG] timeout ret=%d", ret);
            break;
        }

        LOG_INF("[LOGGER-MSG] thread=%s seq=%u temp=%d latency=%ums",
                k_thread_name_get(k_current_get()),
                msg.seq,
                msg.temperature_mc,
                k_uptime_get_32() - msg.timestamp_ms);

        k_msleep(350);
        task_wdt_feed(wdt_logger);
    }
    task_wdt_delete(wdt_logger);

    logger_thread_finished = true;
}

static void health_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    while (1)
    {
        k_msleep(50);

        uint32_t used = k_msgq_num_used_get(&pipeline_queue);

        LOG_INF("[HEALTH] pipeline_queue=%u/%u", used, PIPELINE_QUEUE_DEPTH);

        LOG_DBG("[HEALTH] queue has %u free slots", PIPELINE_QUEUE_DEPTH - used);

        if (used > 0.75 * PIPELINE_QUEUE_DEPTH) 
        {
            LOG_WRN("[HEALTH] pipeline_queue is more than 75%% full");
        }

        if ((true == sensor_thread_finished ) && (true == logger_thread_finished))
        {
            break;
        }
    }

    LOG_INF("[HEALTH] done");
}

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_thread_fn, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(logger_thread, STACK_SIZE, logger_thread_fn, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(health_thread, STACK_SIZE, health_thread_fn, NULL, NULL, NULL, PRIO, 0, 0);

int main(void)
{
    task_wdt_init(NULL);
    return 0;
}
