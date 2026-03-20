#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TheBinaryBjorn");
MODULE_DESCRIPTION("SSD1306 OLED driver");

// Store the i2c_client pointer for use across functions
static struct i2c_client *oled_client;

// Send a single command byte to the SSD1306
static int ssd1306_write_cmd(struct i2c_client *client, u8 cmd)
{
    u8 buf[2] = { 0x00, cmd };  // 0x00 = command mode, cmd = the command
    int ret = i2c_master_send(client, buf, 2);
    if (ret < 0) {
        dev_err(&client->dev, "failed to send command 0x%02X: %d\n", cmd, ret);
        return ret;
    }
    return 0;
}

// Send the full init sequence from the SSD1306 datasheet
static int ssd1306_init_display(struct i2c_client *client)
{
    int ret;

    // turn display off before configuring
    ret = ssd1306_write_cmd(client, 0xAE); if (ret) return ret;

    // set display clock divide ratio / oscillator frequency
    ret = ssd1306_write_cmd(client, 0xD5); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x80); if (ret) return ret;

    // set multiplex ratio (64 rows)
    ret = ssd1306_write_cmd(client, 0xA8); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x3F); if (ret) return ret;

    // set display offset
    ret = ssd1306_write_cmd(client, 0xD3); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x00); if (ret) return ret;

    // set start line to 0
    ret = ssd1306_write_cmd(client, 0x40); if (ret) return ret;

    // enable charge pump
    ret = ssd1306_write_cmd(client, 0x8D); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x14); if (ret) return ret;

    // set memory addressing mode to horizontal
    ret = ssd1306_write_cmd(client, 0x20); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x00); if (ret) return ret;

    // set segment remap (mirrors display horizontally)
    ret = ssd1306_write_cmd(client, 0xA1); if (ret) return ret;

    // set COM output scan direction (mirrors vertically)
    ret = ssd1306_write_cmd(client, 0xC8); if (ret) return ret;

    // set COM pins hardware config
    ret = ssd1306_write_cmd(client, 0xDA); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x12); if (ret) return ret;

    // set contrast
    ret = ssd1306_write_cmd(client, 0x81); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xCF); if (ret) return ret;

    // set precharge period
    ret = ssd1306_write_cmd(client, 0xD9); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xF1); if (ret) return ret;

    // set VCOMH deselect level
    ret = ssd1306_write_cmd(client, 0xDB); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x40); if (ret) return ret;

    // enable display output (not all RAM)
    ret = ssd1306_write_cmd(client, 0xA4); if (ret) return ret;

    // set normal display (not inverted)
    ret = ssd1306_write_cmd(client, 0xA6); if (ret) return ret;

    // turn display on
    ret = ssd1306_write_cmd(client, 0xAF); if (ret) return ret;

    dev_info(&client->dev, "display initialised\n");
    return 0;
}

static int ssd1306_probe(struct i2c_client *client)
{
    int ret;

    dev_info(&client->dev, "found display at address 0x%02X\n", client->addr);

    oled_client = client;

    ret = ssd1306_init_display(client);
    if (ret) {
        dev_err(&client->dev, "display init failed\n");
        return ret;
    }

    return 0;
}

static void ssd1306_remove(struct i2c_client *client)
{
    // turn display off cleanly on unload
    ssd1306_write_cmd(client, 0xAE);
    dev_info(&client->dev, "display removed\n");
}

static const struct i2c_device_id ssd1306_id[] = {
    { "ssd1306", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ssd1306_id);

static struct i2c_driver ssd1306_driver = {
    .driver = {
        .name  = "ssd1306",
        .owner = THIS_MODULE,
    },
    .probe    = ssd1306_probe,
    .remove   = ssd1306_remove,
    .id_table = ssd1306_id,
};

module_i2c_driver(ssd1306_driver);