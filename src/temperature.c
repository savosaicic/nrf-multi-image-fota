#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(temp_sensor);

/* STTS22H Operating temperature */
#define STTS22H_TEMP_MIN -40.0
#define STTS22H_TEMP_MAX 125.0

static const struct device *stts22h = DEVICE_DT_GET(DT_NODELABEL(stts22h));

static struct k_work_delayable temp_work;

#define TEMP_UPDATE_INTERVAL_S 10

static void temp_work_handler(struct k_work *work)
{
  struct sensor_value temperature;
  double              dtemp;
  int                 ret;

  ret = sensor_sample_fetch(stts22h);
  if (ret < 0) {
    LOG_ERR("Failed to fetch temperature sample: %d", ret);
    goto reschedule;
  }

  ret = sensor_channel_get(stts22h, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
  if (ret < 0) {
    LOG_ERR("Failed to get temperature data: %d", ret);
    goto reschedule;
  }

  dtemp = sensor_value_to_double(&temperature);
  ret   = lwm2m_set_f64(&LWM2M_OBJ(3303, 0, 5700), dtemp);
  if (ret < 0) {
    LOG_ERR("Failed to update temperature: %d", ret);
  } else {
    LOG_DBG("Temperature updated: %.1f C", dtemp);
  }

reschedule:
  k_work_schedule(&temp_work, K_SECONDS(TEMP_UPDATE_INTERVAL_S));
}

int setup_temperature_sensor(void)
{
  int ret;

  if (!device_is_ready(stts22h)) {
    LOG_ERR("STTS22H not ready");
    return -ENODEV;
  }

  ret = lwm2m_create_object_inst(&LWM2M_OBJ(3303, 0));
  if (ret < 0 && ret != -EEXIST) {
    LOG_ERR("Failed to create Temperature object instance: %d", ret);
    return ret;
  }

  ret = lwm2m_set_f64(&LWM2M_OBJ(3303, 0, 5603), STTS22H_TEMP_MIN);
  if (ret < 0) {
    LOG_ERR("Failed to set temp min range: %d", ret);
    return ret;
  }

  ret = lwm2m_set_f64(&LWM2M_OBJ(3303, 0, 5604), STTS22H_TEMP_MAX);
  if (ret < 0) {
    LOG_ERR("Failed to set temp max range: %d", ret);
    return ret;
  }

  ret = lwm2m_set_string(&LWM2M_OBJ(3303, 0, 5701), "Celsius");
  if (ret < 0) {
    LOG_ERR("Failed to set sensor units: %d", ret);
    return ret;
  }

  k_work_init_delayable(&temp_work, temp_work_handler);
  k_work_schedule(&temp_work, K_NO_WAIT);

  return 0;
}
