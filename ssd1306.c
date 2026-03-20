// ssd1306.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TheBinaryBjorn");
MODULE_DESCRIPTION("SSD1306 OLED driver");

// Called by the kernel when it finds a matching I2C device
static int ssd1306_probe(struct i2c_client *client)
{
    pr_info("ssd1306: found display at address 0x%02X\n", client->addr);
    return 0;
}

// Called when the driver is unloaded or device removed
static void ssd1306_remove(struct i2c_client *client)
{
    pr_info("ssd1306: display removed\n");
}

// Tells the kernel which devices this driver can handle
static const struct i2c_device_id ssd1306_id[] = {
    { "ssd1306", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ssd1306_id);

static struct i2c_driver ssd1306_driver = {
    .driver = {
        .name   = "ssd1306",
        .owner  = THIS_MODULE,
    },
    .probe    = ssd1306_probe,
    .remove   = ssd1306_remove,
    .id_table = ssd1306_id,
};

module_i2c_driver(ssd1306_driver);