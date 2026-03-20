#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TheBinaryBjorn");
MODULE_DESCRIPTION("SSD1306 OLED character device driver");

#define DEVICE_NAME     "oled"
#define CLASS_NAME      "oled"
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGES      8       // 64 rows / 8 bits per page
#define FONT_WIDTH      6       // 5 pixels + 1 spacing
#define FONT_HEIGHT     8       // one full page tall
#define MAX_CHARS       (OLED_WIDTH / FONT_WIDTH)  // 21 chars per line

// -------------------------------------------------------
// 5x7 font table — each char is 5 bytes, one bit per pixel
// index 0 = ASCII 32 (space), index 1 = '!' etc.
// -------------------------------------------------------
static const u8 font5x7[][5] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, // space
    { 0x00, 0x00, 0x5F, 0x00, 0x00 }, // !
    { 0x00, 0x07, 0x00, 0x07, 0x00 }, // "
    { 0x14, 0x7F, 0x14, 0x7F, 0x14 }, // #
    { 0x24, 0x2A, 0x7F, 0x2A, 0x12 }, // $
    { 0x23, 0x13, 0x08, 0x64, 0x62 }, // %
    { 0x36, 0x49, 0x55, 0x22, 0x50 }, // &
    { 0x00, 0x05, 0x03, 0x00, 0x00 }, // '
    { 0x00, 0x1C, 0x22, 0x41, 0x00 }, // (
    { 0x00, 0x41, 0x22, 0x1C, 0x00 }, // )
    { 0x08, 0x2A, 0x1C, 0x2A, 0x08 }, // *
    { 0x08, 0x08, 0x3E, 0x08, 0x08 }, // +
    { 0x00, 0x50, 0x30, 0x00, 0x00 }, // ,
    { 0x08, 0x08, 0x08, 0x08, 0x08 }, // -
    { 0x00, 0x60, 0x60, 0x00, 0x00 }, // .
    { 0x20, 0x10, 0x08, 0x04, 0x02 }, // /
    { 0x3E, 0x51, 0x49, 0x45, 0x3E }, // 0
    { 0x00, 0x42, 0x7F, 0x40, 0x00 }, // 1
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, // 2
    { 0x21, 0x41, 0x45, 0x4B, 0x31 }, // 3
    { 0x18, 0x14, 0x12, 0x7F, 0x10 }, // 4
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, // 5
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, // 6
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, // 7
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, // 8
    { 0x06, 0x49, 0x49, 0x29, 0x1E }, // 9
    { 0x00, 0x36, 0x36, 0x00, 0x00 }, // :
    { 0x00, 0x56, 0x36, 0x00, 0x00 }, // ;
    { 0x00, 0x08, 0x14, 0x22, 0x41 }, // 
    { 0x14, 0x14, 0x14, 0x14, 0x14 }, // =
    { 0x41, 0x22, 0x14, 0x08, 0x00 }, // >
    { 0x02, 0x01, 0x51, 0x09, 0x06 }, // ?
    { 0x32, 0x49, 0x79, 0x41, 0x3E }, // @
    { 0x7E, 0x11, 0x11, 0x11, 0x7E }, // A
    { 0x7F, 0x49, 0x49, 0x49, 0x36 }, // B
    { 0x3E, 0x41, 0x41, 0x41, 0x22 }, // C
    { 0x7F, 0x41, 0x41, 0x22, 0x1C }, // D
    { 0x7F, 0x49, 0x49, 0x49, 0x41 }, // E
    { 0x7F, 0x09, 0x09, 0x09, 0x01 }, // F
    { 0x3E, 0x41, 0x41, 0x49, 0x7A }, // G
    { 0x7F, 0x08, 0x08, 0x08, 0x7F }, // H
    { 0x00, 0x41, 0x7F, 0x41, 0x00 }, // I
    { 0x20, 0x40, 0x41, 0x3F, 0x01 }, // J
    { 0x7F, 0x08, 0x14, 0x22, 0x41 }, // K
    { 0x7F, 0x40, 0x40, 0x40, 0x40 }, // L
    { 0x7F, 0x02, 0x04, 0x02, 0x7F }, // M
    { 0x7F, 0x04, 0x08, 0x10, 0x7F }, // N
    { 0x3E, 0x41, 0x41, 0x41, 0x3E }, // O
    { 0x7F, 0x09, 0x09, 0x09, 0x06 }, // P
    { 0x3E, 0x41, 0x51, 0x21, 0x5E }, // Q
    { 0x7F, 0x09, 0x19, 0x29, 0x46 }, // R
    { 0x46, 0x49, 0x49, 0x49, 0x31 }, // S
    { 0x01, 0x01, 0x7F, 0x01, 0x01 }, // T
    { 0x3F, 0x40, 0x40, 0x40, 0x3F }, // U
    { 0x1F, 0x20, 0x40, 0x20, 0x1F }, // V
    { 0x3F, 0x40, 0x38, 0x40, 0x3F }, // W
    { 0x63, 0x14, 0x08, 0x14, 0x63 }, // X
    { 0x07, 0x08, 0x70, 0x08, 0x07 }, // Y
    { 0x61, 0x51, 0x49, 0x45, 0x43 }, // Z
    { 0x00, 0x00, 0x7F, 0x41, 0x41 }, // [
    { 0x02, 0x04, 0x08, 0x10, 0x20 }, // backslash
    { 0x41, 0x41, 0x7F, 0x00, 0x00 }, // ]
    { 0x04, 0x02, 0x01, 0x02, 0x04 }, // ^
    { 0x40, 0x40, 0x40, 0x40, 0x40 }, // _
    { 0x00, 0x01, 0x02, 0x04, 0x00 }, // `
    { 0x20, 0x54, 0x54, 0x54, 0x78 }, // a
    { 0x7F, 0x48, 0x44, 0x44, 0x38 }, // b
    { 0x38, 0x44, 0x44, 0x44, 0x20 }, // c
    { 0x38, 0x44, 0x44, 0x48, 0x7F }, // d
    { 0x38, 0x54, 0x54, 0x54, 0x18 }, // e
    { 0x08, 0x7E, 0x09, 0x01, 0x02 }, // f
    { 0x08, 0x54, 0x54, 0x54, 0x3C }, // g
    { 0x7F, 0x08, 0x04, 0x04, 0x78 }, // h
    { 0x00, 0x44, 0x7D, 0x40, 0x00 }, // i
    { 0x20, 0x40, 0x44, 0x3D, 0x00 }, // j
    { 0x7F, 0x10, 0x28, 0x44, 0x00 }, // k
    { 0x00, 0x41, 0x7F, 0x40, 0x00 }, // l
    { 0x7C, 0x04, 0x18, 0x04, 0x78 }, // m
    { 0x7C, 0x08, 0x04, 0x04, 0x78 }, // n
    { 0x38, 0x44, 0x44, 0x44, 0x38 }, // o
    { 0x7C, 0x14, 0x14, 0x14, 0x08 }, // p
    { 0x08, 0x14, 0x14, 0x18, 0x7C }, // q
    { 0x7C, 0x08, 0x04, 0x04, 0x08 }, // r
    { 0x48, 0x54, 0x54, 0x54, 0x20 }, // s
    { 0x04, 0x3F, 0x44, 0x40, 0x20 }, // t
    { 0x3C, 0x40, 0x40, 0x40, 0x7C }, // u
    { 0x1C, 0x20, 0x40, 0x20, 0x1C }, // v
    { 0x3C, 0x40, 0x30, 0x40, 0x3C }, // w
    { 0x44, 0x28, 0x10, 0x28, 0x44 }, // x
    { 0x0C, 0x50, 0x50, 0x50, 0x3C }, // y
    { 0x44, 0x64, 0x54, 0x4C, 0x44 }, // z
    { 0x00, 0x08, 0x36, 0x41, 0x00 }, // {
    { 0x00, 0x00, 0x7F, 0x00, 0x00 }, // |
    { 0x00, 0x41, 0x36, 0x08, 0x00 }, // }
    { 0x08, 0x08, 0x2A, 0x1C, 0x08 }, // ~
};

// -------------------------------------------------------
// Driver state — everything we need to keep track of
// -------------------------------------------------------
struct ssd1306_dev {
    struct i2c_client   *client;
    struct cdev          cdev;
    u8                   framebuffer[OLED_PAGES * OLED_WIDTH];
};

static struct ssd1306_dev *oled_dev;
static dev_t               dev_num;
static struct class       *oled_class;

// -------------------------------------------------------
// I2C helpers 
// -------------------------------------------------------
static int ssd1306_write_cmd(struct i2c_client *client, u8 cmd)
{
    u8 buf[2] = { 0x00, cmd };
    int ret = i2c_master_send(client, buf, 2);
    return ret < 0 ? ret : 0;
}

// Push the entire framebuffer to the display over I2C
static int ssd1306_flush(struct ssd1306_dev *dev)
{
    int page, ret;
    u8 buf[OLED_WIDTH + 1];

    for (page = 0; page < OLED_PAGES; page++) {
        // tell the controller which page and column to write to
        ret = ssd1306_write_cmd(dev->client, 0xB0 + page); // set page address
        if (ret) return ret;
        ret = ssd1306_write_cmd(dev->client, 0x00);         // column low nibble
        if (ret) return ret;
        ret = ssd1306_write_cmd(dev->client, 0x10);         // column high nibble
        if (ret) return ret;

        // send the 128 bytes for this page in one I2C transaction
        buf[0] = 0x40;  // data mode
        memcpy(&buf[1], &dev->framebuffer[page * OLED_WIDTH], OLED_WIDTH);
        ret = i2c_master_send(dev->client, buf, OLED_WIDTH + 1);
        if (ret < 0) return ret;
    }
    return 0;
}

// -------------------------------------------------------
// Font rendering — convert a char to pixels in framebuffer
// -------------------------------------------------------
static void ssd1306_draw_char(struct ssd1306_dev *dev, char c, int col, int page)
{
    int i;
    const u8 *glyph;

    // clamp to printable ASCII range
    if (c < 32 || c > 126)
        c = 32;  // replace unknown chars with space

    glyph = font5x7[c - 32];  // offset by 32 since table starts at space

    for (i = 0; i < 5; i++) {
        if (col + i < OLED_WIDTH)
            dev->framebuffer[page * OLED_WIDTH + col + i] = glyph[i];
    }
    // sixth column is always blank spacing between characters
    if (col + 5 < OLED_WIDTH)
        dev->framebuffer[page * OLED_WIDTH + col + 5] = 0x00;
}

static void ssd1306_render_string(struct ssd1306_dev *dev, const char *str, int len)
{
    int i, col = 0;

    // clear framebuffer first
    memset(dev->framebuffer, 0, sizeof(dev->framebuffer));

    for (i = 0; i < len && col < OLED_WIDTH; i++) {
        if (str[i] == '\n') {
            // newline not supported yet, just stop
            break;
        }
        ssd1306_draw_char(dev, str[i], col, 0);  // page 0 = top of screen
        col += FONT_WIDTH;
    }
}

// -------------------------------------------------------
// Character device file operations
// -------------------------------------------------------
static int oled_open(struct inode *inode, struct file *file)
{
    // store our device struct in the file handle for other ops to use
    file->private_data = container_of(inode->i_cdev, struct ssd1306_dev, cdev);
    return 0;
}

static int oled_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t oled_write(struct file *file, const char __user *buf,
                           size_t count, loff_t *ppos)
{
    struct ssd1306_dev *dev = file->private_data;
    char kbuf[MAX_CHARS + 1];
    size_t len;
    int ret;

    // cap how much we'll accept
    len = min(count, (size_t)MAX_CHARS);

    // copy from userspace into kernel buffer — never trust userspace pointers directly
    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    // strip trailing newline that echo adds
    if (len > 0 && kbuf[len - 1] == '\n')
        len--;

    kbuf[len] = '\0';

    ssd1306_render_string(dev, kbuf, len);

    ret = ssd1306_flush(dev);
    if (ret)
        return ret;

    dev_info(&dev->client->dev, "displayed: %s\n", kbuf);
    return count;  // tell userspace we consumed all the bytes
}

static const struct file_operations oled_fops = {
    .owner   = THIS_MODULE,
    .open    = oled_open,
    .release = oled_release,
    .write   = oled_write,
};

// -------------------------------------------------------
// Init sequence — same as Step 3
// -------------------------------------------------------
static int ssd1306_init_display(struct i2c_client *client)
{
    int ret;
    ret = ssd1306_write_cmd(client, 0xAE); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xD5); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x80); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xA8); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x3F); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xD3); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x00); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x40); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x8D); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x14); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x20); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x00); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xA1); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xC8); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xDA); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x12); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x81); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xCF); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xD9); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xF1); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xDB); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0x40); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xA4); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xA6); if (ret) return ret;
    ret = ssd1306_write_cmd(client, 0xAF); if (ret) return ret;
    dev_info(&client->dev, "display initialised\n");
    return 0;
}

// -------------------------------------------------------
// probe — now registers the character device too
// -------------------------------------------------------
static int ssd1306_probe(struct i2c_client *client)
{
    int ret;

    dev_info(&client->dev, "found display at 0x%02X\n", client->addr);

    // allocate our device state
    oled_dev = devm_kzalloc(&client->dev, sizeof(*oled_dev), GFP_KERNEL);
    if (!oled_dev)
        return -ENOMEM;

    oled_dev->client = client;

    // initialise the hardware
    ret = ssd1306_init_display(client);
    if (ret)
        return ret;

    // allocate a major/minor number dynamically
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;

    // initialise and register the character device
    cdev_init(&oled_dev->cdev, &oled_fops);
    oled_dev->cdev.owner = THIS_MODULE;
    ret = cdev_add(&oled_dev->cdev, dev_num, 1);
    if (ret)
        goto err_cdev;

    // create /dev/oled automatically via udev
    oled_class = class_create(CLASS_NAME);
    if (IS_ERR(oled_class)) {
        ret = PTR_ERR(oled_class);
        goto err_class;
    }

    if (IS_ERR(device_create(oled_class, NULL, dev_num, NULL, DEVICE_NAME))) {
        ret = -EINVAL;
        goto err_device;
    }

    dev_info(&client->dev, "/dev/oled ready\n");
    return 0;

    // goto error handling — unwind in reverse order of allocation
err_device:
    class_destroy(oled_class);
err_class:
    cdev_del(&oled_dev->cdev);
err_cdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void ssd1306_remove(struct i2c_client *client)
{
    device_destroy(oled_class, dev_num);
    class_destroy(oled_class);
    cdev_del(&oled_dev->cdev);
    unregister_chrdev_region(dev_num, 1);
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