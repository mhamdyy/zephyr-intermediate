#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/task_wdt/task_wdt.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE (2048)
#define PRIO (7)

typedef struct _sensor_data {
    int32_t temperature_mc;
    uint32_t timestamp_ms;
    uint8_t seq;
}sensor_data;

void display_listener_cb(const struct zbus_channel *chan);

ZBUS_LISTENER_DEFINE(fast_display_listner, display_listener_cb);

ZBUS_MSG_SUBSCRIBER_DEFINE(logger_sub);

ZBUS_CHAN_DEFINE(sensor_channel, struct _sensor_data, NULL, NULL,
                 ZBUS_OBSERVERS(fast_display_listner, logger_sub),
                 ZBUS_MSG_INIT(.temperature_mc = 0,
                               .timestamp_ms = 0,
                               .seq = 0));

void display_listener_cb(const struct zbus_channel *chan)
{
    const sensor_data *msg = (const sensor_data *)zbus_chan_const_msg(chan);

    LOG_INF("[DISPLAY-LISTNER] thread=%s seq=%u temp=%d mC",
            k_thread_name_get(k_current_get()),
            msg->seq,
            msg->temperature_mc);
}

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

    for (int i =0; i< 10;i++)
    {
        data.temperature_mc = 24000 + (i * 350);
        data.timestamp_ms = k_uptime_get_32();
        data.seq = (uint8_t)i;

        LOG_INF("[SENSOR] publish seq=%u temp=%d mC",
                data.seq,
                data.temperature_mc);

        int ret = zbus_chan_pub(&sensor_channel, &data, K_MSEC(100));

        if (ret != 0)
        {
            LOG_WRN("[SENSOR] publish failed ret=%d", ret);
        }
        k_msleep(100);
        task_wdt_feed(wdt_sensor);
    }
    task_wdt_delete(wdt_sensor);
}

void logger_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "logger");

    int wdt_logger = task_wdt_add(2000, logger_wgt_miss, (void*) k_current_get());

    const struct zbus_channel *chan;

    while (1)
    {
        sensor_data msg;

        int ret = zbus_sub_wait_msg(&logger_sub, &chan, &msg, K_MSEC(1500));

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

        k_msleep(3500);
        task_wdt_feed(wdt_logger);
    }
    task_wdt_delete(wdt_logger);

}

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_thread_fn, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(logger_thread, STACK_SIZE, logger_thread_fn, NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(health_thread, STACK_SIZE, health_thread_fn, NULL, NULL, NULL, PRIO, 0, 0);

int main(void)
{
    task_wdt_init(NULL);
    return 0;
}
