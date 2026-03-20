# Linux Kernel Driver 
# SSD1306 Linux Kernel Driver

A loadable Linux kernel module that drives an SSD1306 OLED display over I2C, exposing a character device interface at `/dev/oled`. Text written to the device is rendered using a 5x7 bitmap font and flushed to the physical display.

```bash
echo "Hello Jarvis" > /dev/oled
```

---

## Hardware

| Component | Detail |
|-----------|--------|
| Display | SSD1306 128x64 OLED |
| Interface | I2C |
| I2C address | `0x3C` |
| Platform | Raspberry Pi (BCM2711, ARM64) |
| Kernel | Linux 6.x |

**Wiring**

| OLED pin | Pi pin |
|----------|--------|
| GND | Ground |
| VDD | 3.3V |
| SCK | GPIO 3 (SCL) |
| SDA | GPIO 2 (SDA) |

---

## Architecture

```
userspace
  echo "Hello" > /dev/oled
        │
        ▼
   VFS layer
        │
        ▼
  oled_write()
  copy_from_user()        ← kernel/userspace boundary
        │
        ▼
  ssd1306_render_string()
  ssd1306_draw_char()     ← ASCII → 5x7 bitmask → framebuffer (1024 bytes in RAM)
        │
        ▼
  ssd1306_flush()         ← pushes framebuffer to hardware page by page
        │
        ▼
  i2c_master_send()       ← I2C bus transaction
        │
        ▼
  SSD1306 controller      ← pixels light up
```

The framebuffer is a 1024-byte array (`8 pages × 128 columns`) held in kernel RAM. All rendering happens in RAM first — nothing touches the hardware until `ssd1306_flush()` is called, sending all 1024 bytes in 8 I2C transactions.

---

## Key kernel APIs used

| API | Purpose |
|-----|---------|
| `i2c_driver` / `i2c_client` | I2C bus framework — device matching and communication |
| `alloc_chrdev_region` | Dynamic major/minor number allocation |
| `cdev_init` / `cdev_add` | Character device registration |
| `class_create` / `device_create` | Automatic `/dev/oled` creation via udev |
| `copy_from_user` | Safe userspace → kernel memory copy |
| `devm_kzalloc` | Device-managed memory allocation |
| `file_operations` | VFS hook table — open, write, release |
| `container_of` | Recover outer struct pointer from embedded member |
| `i2c_master_send` | Raw I2C byte transmission |

---

## Building

**Dependencies**
```bash
sudo apt install linux-headers-$(uname -r) build-essential
```

**Compile**
```bash
make
```

**Cross-compile for Raspberry Pi (from x86 host)**
```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

---

## Usage

**Load the driver**
```bash
sudo insmod ssd1306.ko
echo ssd1306 0x3c | sudo tee /sys/bus/i2c/devices/i2c-1/new_device
sudo chmod 666 /dev/oled
```

**Write text to the display**
```bash
echo "Hello Jarvis" > /dev/oled
```

**Check kernel logs**
```bash
dmesg | tail -10
```

Expected output:
```
ssd1306 0-003c: found display at 0x3C
ssd1306 0-003c: display initialised
ssd1306 0-003c: /dev/oled ready
ssd1306 0-003c: displayed: Hello Jarvis
```

**Unload the driver**
```bash
echo 0x3c | sudo tee /sys/bus/i2c/devices/i2c-1/delete_device
sudo rmmod ssd1306
```

The display turns off cleanly on unload — `ssd1306_remove()` sends the `0xAE` power-off command before the module exits.

---

## Project structure

```
.
├── ssd1306.c       # full driver — i2c, framebuffer, character device, font rendering
├── Makefile        # kernel module build rules
└── README.md
```

---

## What I learned

- Linux kernel module lifecycle (`module_init` / `module_exit`)
- I2C driver framework — `i2c_driver`, `probe`/`remove`, `MODULE_DEVICE_TABLE`
- Reading hardware datasheets — SSD1306 init sequence from section 10.1/10.3
- Character device registration and the VFS hook system
- Kernel/userspace memory boundary — `copy_from_user`
- Device-managed memory with `devm_kzalloc`
- Proper kernel error handling with `goto` unwind chains
- Framebuffer concepts — page-based display memory layout
- Bitmap font rendering — 5x7 glyph table, bit-per-pixel layout
- Git branching and conventional commit workflow for kernel development

---

## Author

**TheBinaryBjorn**