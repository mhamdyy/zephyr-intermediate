/*
 * temp_alarm - Error-path unit tests (homework skeleton)
 *
 * Task 2: fill in the stubs according to TEST_SPEC.md. Each stub calls
 * ztest_test_skip(), so the binary builds and runs cleanly until the test is
 * implemented.
 *
 * Run:
 *   west twister -T tests/temp_alarm -p native_sim
 */

#include <errno.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <sys/types.h>

#include "sensor_fake.h"
#include "temp_alarm.h"

#define DEFAULT_THRESHOLD 30

/* Same before() hook as in test_basic.c: a clean fake and an initialised module. */
static void before(void *fixture)
{
    ARG_UNUSED(fixture);

    sensor_fake_reset();
    sensor_fake_init();

    zassume_ok(temp_alarm_init(&fake_sensor_dev, DEFAULT_THRESHOLD),
           "precondition: temp_alarm_init must succeed");
}

/*
 * ============================================================================
 * Test Suite: temp_alarm_sensor_errors
 *
 * Failures from the sensor driver, injected through the fakes.
 * ============================================================================
 */
ZTEST_SUITE(temp_alarm_sensor_errors, NULL, NULL, before, NULL, NULL);

ZTEST(temp_alarm_sensor_errors, test_fetch_error_propagates)
{
    struct temp_alarm_status status;

    /*Simulate a failed sensor fetch channel*/
    fake_sensor_sample_fetch_fake.return_val = -EIO;

    /*Simulate a temperature reading below the threshold (20 C)*/
    sensor_fake_set_temperature(20);

    /*Call Temperature Alarm Check */
    zassert_equal(temp_alarm_check(), -EIO, "temp_alarm_check must return -EIO on sensor fetch error");

    zassert_equal(fake_sensor_sample_fetch_fake.call_count, 1, "sensor sample fetch must be called once, got %u", fake_sensor_sample_fetch_fake.call_count);

    zassert_equal(fake_sensor_sample_fetch_fake.arg1_val, SENSOR_CHAN_AMBIENT_TEMP, "sensor sample fetch must be called with SENSOR_CHAN_AMBIENT_TEMP, got %d",
         fake_sensor_sample_fetch_fake.arg1_val);

    zassert_equal(fake_sensor_channel_get_fake.call_count, 0, "sensor channel get must not be called, got %u", fake_sensor_channel_get_fake.call_count);

    /*Get Alarm Status*/
    zassert_ok(temp_alarm_get_status(&status), "get_status must succeed");

    zassert_equal(status.last_temp, 0, "last_temp must be 0 C, got %d", status.last_temp);

    zassert_false(status.is_alarming, "is_alarming must be false");

    zassert_equal(status.last_error, -EIO, "last_error must be -EIO, got %d", status.last_error);
}

ZTEST(temp_alarm_sensor_errors, test_channel_get_error_propagates)
{
    struct temp_alarm_status status;

    /*Simulate a successful sensor fetch channel*/
    fake_sensor_sample_fetch_fake.return_val = 0;

    /*Unset the custom fake function for sensor channel get*/
    fake_sensor_channel_get_fake.custom_fake = NULL;

    /*Simulate a failed sensor channel get*/
    fake_sensor_channel_get_fake.return_val = -EIO;

    /*Call Temperature Alarm Check */
    zassert_equal(temp_alarm_check(), -EIO, "temp_alarm_check must return -EIO on sensor fetch error");

    zassert_equal(fake_sensor_sample_fetch_fake.call_count, 1, "sensor sample fetch must be called once, got %u", fake_sensor_sample_fetch_fake.call_count);

    zassert_equal(fake_sensor_sample_fetch_fake.arg1_val, SENSOR_CHAN_AMBIENT_TEMP, "sensor sample fetch must be called with SENSOR_CHAN_AMBIENT_TEMP, got %d",
         fake_sensor_sample_fetch_fake.arg1_val);

    zassert_equal(fake_sensor_channel_get_fake.call_count, 1, "sensor channel get must be called once, got %u", fake_sensor_channel_get_fake.call_count);

    zassert_equal(fake_sensor_channel_get_fake.arg1_val, SENSOR_CHAN_AMBIENT_TEMP, "sensor channel get must be called with SENSOR_CHAN_AMBIENT_TEMP, got %d", 
        fake_sensor_channel_get_fake.arg1_val);

    /*Get Alarm Status*/
    zassert_ok(temp_alarm_get_status(&status), "get_status must succeed");

    zassert_equal(status.last_temp, 0, "last_temp must be 0 C, got %d", status.last_temp);

    zassert_false(status.is_alarming, "is_alarming must be false");

    zassert_equal(status.last_error, -EIO, "last_error must be -EIO, got %d", status.last_error);
}

ZTEST(temp_alarm_sensor_errors, test_fail_then_recover)
{
    struct temp_alarm_status status;

    /*Simulate a failed sensor fetch channel then a successful sensor fetch*/
    static ssize_t seq[] = {-EIO, 0};

    SET_RETURN_SEQ(fake_sensor_sample_fetch, seq, ARRAY_SIZE(seq));

    /*Simulate a temperature reading below the threshold (20 C)*/
    sensor_fake_set_temperature(20);

    /*Call Temperature Alarm Check */
    zassert_equal(temp_alarm_check(), -EIO, "temp_alarm_check must return -EIO on sensor fetch error");

    zassert_equal(fake_sensor_sample_fetch_fake.call_count, 1, "sensor sample fetch must be called once, got %u", fake_sensor_sample_fetch_fake.call_count);

    zassert_equal(fake_sensor_sample_fetch_fake.arg1_val, SENSOR_CHAN_AMBIENT_TEMP, "sensor sample fetch must be called with SENSOR_CHAN_AMBIENT_TEMP, got %d",
         fake_sensor_sample_fetch_fake.arg1_val);

    zassert_equal(fake_sensor_channel_get_fake.call_count, 0, "sensor channel get must not be called, got %u", fake_sensor_channel_get_fake.call_count);

    /*Get Alarm Status*/
    zassert_ok(temp_alarm_get_status(&status), "get_status must succeed");

    zassert_equal(status.last_temp, 0, "last_temp must be 0 C, got %d", status.last_temp);

    zassert_false(status.is_alarming, "is_alarming must be false");

    zassert_equal(status.last_error, -EIO, "last_error must be -EIO, got %d", status.last_error);

    /*Call Temperature Alarm Check */
    zassert_ok(temp_alarm_check(), "temp_alarm_check must succeed after recovery");

    zassert_equal(fake_sensor_sample_fetch_fake.call_count, 2, "sensor sample fetch must be called twice, got %u", fake_sensor_sample_fetch_fake.call_count);

    zassert_equal(fake_sensor_sample_fetch_fake.arg1_val, SENSOR_CHAN_AMBIENT_TEMP, "sensor sample fetch must be called with SENSOR_CHAN_AMBIENT_TEMP, got %d",
         fake_sensor_sample_fetch_fake.arg1_val);

    zassert_equal(fake_sensor_channel_get_fake.call_count, 1, "sensor channel get must be called once, got %u", fake_sensor_channel_get_fake.call_count);

    zassert_equal(fake_sensor_channel_get_fake.arg1_val, SENSOR_CHAN_AMBIENT_TEMP, "sensor channel get must be called with SENSOR_CHAN_AMBIENT_TEMP, got %d", 
        fake_sensor_channel_get_fake.arg1_val);

    /*Get Alarm Status*/
    zassert_ok(temp_alarm_get_status(&status), "get_status must succeed");

    zassert_equal(status.last_temp, 20, "last_temp must be 20 C, got %d", status.last_temp);

    zassert_false(status.is_alarming, "is_alarming must be false");

    zassert_equal(status.last_error, -EIO, "last_error must be -EIO, got %d", status.last_error);
}

/*
 * ============================================================================
 * Test Suite: temp_alarm_invalid_input
 *
 * Arguments the public API must reject.
 * ============================================================================
 */
ZTEST_SUITE(temp_alarm_invalid_input, NULL, NULL, before, NULL, NULL);

ZTEST(temp_alarm_invalid_input, test_get_status_null)
{
    /*Get Alarm Status*/
    zassert_equal(temp_alarm_get_status(NULL), -EINVAL, "get_status must fail with -EINVAL");
}

ZTEST(temp_alarm_invalid_input, test_init_without_device)
{
    /*Init temp alarm*/
    zassert_equal(temp_alarm_init(NULL, 30), -ENODEV, "init must fail with -ENODEV");
}

ZTEST(temp_alarm_invalid_input, test_init_threshold_out_of_range)
{
    /*Init temp alarm*/
    zassert_equal(temp_alarm_init(&fake_sensor_dev, -1), -EINVAL, "init must fail with -EINVAL");

    /*Init temp alarm*/
    zassert_equal(temp_alarm_init(&fake_sensor_dev, 101), -EINVAL, "init must fail with -EINVAL");
}

ZTEST(temp_alarm_invalid_input, test_set_threshold_out_of_range)
{
    /*Set the temp alarm threshold*/
    zassert_equal(temp_alarm_set_threshold(-1), -EINVAL, "init must fail with -EINVAL");

    /*Init temp alarm*/
    zassert_equal(temp_alarm_set_threshold(101), -EINVAL, "init must fail with -EINVAL");
}
