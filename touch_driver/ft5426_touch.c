// FILE: ft5426_touch.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/property.h>

// Register definitions
#define FT5426_REG_TD_STATUS 0x02
#define FT5426_REG_TOUCH_DATA 0x03
#define DRIVER_NAME "ft5426_touch"

static const struct i2c_device_id ft5426_id[] = {
    { DRIVER_NAME, 0 },
    { }
};

static int ft5426_remove(struct i2c_client *client)
{
    struct ft5426_data *data = i2c_get_clientdata(client);
    input_unregister_device(data->input_dev);
    return 0;
}


struct ft5426_data {
  struct i2c_client *client;
  struct input_dev *input_dev;
  int irq_gpio;
  int reset_gpio;
  int screen_width;
  int screen_height;
};

static irqreturn_t ft5426_interrupt(int irq, void *dev_id) {
  struct ft5426_data *data = dev_id;
  u8 touch_status;
  int ret;

  ret = i2c_smbus_read_byte_data(data->client, FT5426_REG_TD_STATUS);
  if (ret < 0) return IRQ_HANDLED;

  touch_status = ret & 0x0F;

  if (touch_status > 0) {
    u8 touch_data[6];
    ret = i2c_smbus_read_i2c_block_data(data->client, FT5426_REG_TOUCH_DATA, 6,
                                        touch_data);
    if (ret == 6) {
      int x = ((touch_data[0] & 0x0F) << 8) | touch_data[1];
      int y = ((touch_data[2] & 0x0F) << 8) | touch_data[3];

      input_report_abs(data->input_dev, ABS_X, x);
      input_report_abs(data->input_dev, ABS_Y, y);
      input_report_key(data->input_dev, BTN_TOUCH, 1);
      input_sync(data->input_dev);
    }
  } else {
    input_report_key(data->input_dev, BTN_TOUCH, 0);
    input_sync(data->input_dev);
  }

  return IRQ_HANDLED;
}

static int ft5426_init_hw(struct ft5426_data *data) {
  // Reset sequence
  gpio_set_value(data->reset_gpio, 0);
  msleep(20);
  gpio_set_value(data->reset_gpio, 1);
  msleep(300);

  return 0;
}

static int ft5426_probe(struct i2c_client *client) {
  struct ft5426_data *data;
  struct input_dev *input_dev;
  int ret;

  if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
    return -ENODEV;

  data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
  if (!data) return -ENOMEM;

  input_dev = devm_input_allocate_device(&client->dev);
  if (!input_dev) return -ENOMEM;

  data->client = client;
  data->input_dev = input_dev;

  // Get GPIO and IRQ from device tree
  data->reset_gpio = of_get_named_gpio(client->dev.of_node, "reset-gpios", 0);
  if (!gpio_is_valid(data->reset_gpio)) return -EINVAL;

  ret = devm_gpio_request_one(&client->dev, data->reset_gpio,
                              GPIOF_OUT_INIT_HIGH, "ft5426_reset");
  if (ret) return ret;

  // Get screen dimensions from device tree
  device_property_read_u32(&client->dev, "touchscreen-size-x",
                           &data->screen_width);
  device_property_read_u32(&client->dev, "touchscreen-size-y",
                           &data->screen_height);

  // Setup input device
  input_dev->name = "FT5426 Touchscreen";
  input_dev->id.bustype = BUS_I2C;

  input_set_abs_params(input_dev, ABS_X, 0, data->screen_width - 1, 0, 0);
  input_set_abs_params(input_dev, ABS_Y, 0, data->screen_height - 1, 0, 0);

  __set_bit(EV_ABS, input_dev->evbit);
  __set_bit(EV_KEY, input_dev->evbit);
  __set_bit(BTN_TOUCH, input_dev->keybit);

  ret = ft5426_init_hw(data);
  if (ret) return ret;

  ret = input_register_device(input_dev);
  if (ret) return ret;

  ret = devm_request_threaded_irq(
      &client->dev, client->irq, NULL, ft5426_interrupt,
      IRQF_ONESHOT | IRQF_TRIGGER_FALLING, client->name, data);
  if (ret) return ret;

  i2c_set_clientdata(client, data);
  return 0;
}

// Add OF match table
static const struct of_device_id ft5426_of_match[] = {
    { .compatible = "focaltech,ft5426" },
    { }
};
MODULE_DEVICE_TABLE(of, ft5426_of_match);

static struct i2c_driver ft5426_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(ft5426_of_match),
    },
    .probe = ft5426_probe,
    .remove = ft5426_remove,
    .id_table = ft5426_id,
};