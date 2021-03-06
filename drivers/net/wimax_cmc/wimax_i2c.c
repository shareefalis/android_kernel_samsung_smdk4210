/**
 * wimax_i2c.c
 *
 * EEPROM access functions
 */
#include "headers.h"
#include "download.h"
#include "wimax_i2c.h"
#include "firmware.h"
#include "wimax_plat.h"
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/vmalloc.h>

#define MAX_WIMAXBOOTIMAGE_SIZE		6000
#define MAX_BOOT_WRITE_LENGTH		128
#define WIMAX_EEPROM_ADDRLEN 2


extern struct image_data g_wimax_image;

void wimax_i2c_reset(void);

void eeprom_poweron(void)
{
	mutex_init(&g_wimax_image.lock);
	mutex_lock(&g_wimax_image.lock);

	s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SDA, S3C_GPIO_PULL_NONE);

	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);

	gpio_set_value(I2C_SEL, GPIO_LEVEL_HIGH);    // SEL = 1 to switch i2c-eeprom path to AP
        msleep(10);


	/* power on */
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);

	msleep(100);

	wimax_i2c_reset();
	msleep(10);
}

void eeprom_poweroff(void)
{
	/* power off */
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	gpio_set_value(I2C_SEL, GPIO_LEVEL_LOW); // SEL = 0 to switch i2c-eeprom path to wimax
        msleep(10);


	msleep(100);

	mutex_unlock(&g_wimax_image.lock);
	mutex_destroy(&g_wimax_image.lock);
}

/* I2C functions for read/write EEPROM */

static struct i2c_client *pclient;

void wimax_i2c_reset(void)
{}

int wimax_i2c_write(u_short addr, u_char *data, int length)
{
	int rc;
	int len;
	char data_buffer[MAX_BOOT_WRITE_LENGTH + WIMAX_EEPROM_ADDRLEN];
	struct i2c_msg msg; 
	data_buffer[0] = (unsigned char)((addr >> 8) & 0xff);
	data_buffer[1] = (unsigned char)(addr & 0xff);
	while(length)
		{
			len = (length > MAX_BOOT_WRITE_LENGTH)? MAX_BOOT_WRITE_LENGTH : length ;
			memcpy(data_buffer+WIMAX_EEPROM_ADDRLEN, data, len);
			msg.addr = pclient->addr;
			msg.flags = 0;	//write
			msg.len = (u16)length+WIMAX_EEPROM_ADDRLEN;
			msg.buf = data_buffer;		
			rc = i2c_transfer(pclient->adapter, &msg, 1);
			if (rc < 0) 
				return rc;
			length-=len;
			data+=len;
		}
	return 0;
}

int wimax_i2c_read(u_short addr, u_char *data, int length)
{
	char data_buffer[MAX_BOOT_WRITE_LENGTH + WIMAX_EEPROM_ADDRLEN];
	struct i2c_msg msgs[2];
	data_buffer[0] = (unsigned char)((addr >> 8) & 0xff);
	data_buffer[1] = (unsigned char)(addr & 0xff);
	msgs[0].addr = pclient->addr;
	msgs[0].flags = 0;	//write
	msgs[0].len = WIMAX_EEPROM_ADDRLEN;
	msgs[0].buf = data_buffer;
	msgs[1].addr = pclient->addr;
	msgs[1].flags = I2C_M_RD;	//read
	msgs[1].len = length;
	msgs[1].buf = data;
	return i2c_transfer(pclient->adapter, msgs, 2);
      
}

int wmxeeprom_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	dump_debug("[WMXEEPROM]: probe");
	
	pclient = client;

	return 0;
}

int wmxeeprom_remove(struct i2c_client *client)
{
	dump_debug("[WMXEEPROM]: remove");
	
	return 0;
}

const struct i2c_device_id wmxeeprom_id[]={
	{ "wmxeeprom",0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, wmxeeprom_id);
static struct i2c_driver wmxeeprom_driver = {
	.driver = {		
		.name   = "wmxeeprom",
	},
	.probe 		= wmxeeprom_probe,
	.remove 	= wmxeeprom_remove,
	.id_table	= wmxeeprom_id,

};
int wmxeeprom_init(void)
{
	return i2c_add_driver(&wmxeeprom_driver);
}

void wmxeeprom_exit(void)
{
	i2c_del_driver(&wmxeeprom_driver);
}

int load_wimax_boot(void)
{
	struct	file *fp;
	int	read_size = 0;

	fp = klib_fopen(WIMAX_BOOTIMAGE_PATH, O_RDONLY, 0);

	if (fp) {
		dump_debug("LoadWiMAXBootImage ..");
		g_wimax_image.data = (u_char *)vmalloc(MAX_WIMAXBOOTIMAGE_SIZE);
		if (!g_wimax_image.data) {
			dump_debug("Error: Memory alloc failure.");
			klib_fclose(fp);
			return -1;
		}

		memset(g_wimax_image.data, 0, MAX_WIMAXBOOTIMAGE_SIZE);
		read_size = klib_flen_fcopy(g_wimax_image.data, MAX_WIMAXBOOTIMAGE_SIZE, fp);
		g_wimax_image.size = read_size;
		g_wimax_image.address = 0;
		g_wimax_image.offset = 0;
		mutex_init(&g_wimax_image.lock);

		klib_fclose(fp);
	} else {
		dump_debug("Error: WiMAX image file open failed");
		return -1;
	}

	return 0;
}

int write_rev(void)
{
	u_int	val;
	int retry = 100;
	int err;

	dump_debug("HW REV: %d", system_rev);

	/* swap */
	val = be32_to_cpu(system_rev);

	do
		{
		err = wimax_i2c_write(0x7080, (char *)(&val), 4);
		}
	while(err<0?((retry--)>0):0);
	if(retry<0)
		dump_debug("eeprom error");
	return err ;
}

void erase_cert(void)
{
	char	buf[256] = {0};
	int	len = 4;
	int retry = 100;
	int err;

	do
		{
		err = wimax_i2c_write(0x5800, buf, len);
		}
	while(err<0?((retry--)>0):0);
	if(retry<0)
		dump_debug("eeprom error");
}

int check_cert(void)
{
	char	buf[256] = {0};
	int	len = 16;
	int retry = 100;
	int err;

	do
		{
		err = wimax_i2c_read(0x5800, buf, len);
		}
	while(err<0?((retry--)>0):0);
	if(retry<0)
		dump_debug("eeprom error");

	dump_buffer("Certification", (u_char *)buf, (u_int)len);

	/* check "Cert" */
	if (buf[0] == 0x43 && buf[1] == 0x65 && buf[2] == 0x72 && buf[3] == 0x74)
		return 0;

	return -1;
}

int check_cal(void)
{
	char	buf[256] = {0};
	int	i;
	int	len = 32;
	int retry = 100;
	int err;

	do
		{
		err = wimax_i2c_read(0x5400, buf, len);
		}
	while(err<0?((retry--)>0):0);
	if(retry<0)
		dump_debug("eeprom error");

	dump_buffer("Calibration", (u_char *)buf, (u_int)len);

	/* at lease one buf[i] not equal 0xff means calibrated */
	for (i = 0; i < len; i++)
		if (buf[i] != 0xff)
			return 0;

	/* all 0xff means not calibrated */
	return -1;
}

void eeprom_read_boot()
{
	char	*buf = NULL;
	int	i, j;
	int	len;
	u_int	checksum = 0;
	short	addr = 0;
	int retry = 1000;
	int err;

	eeprom_poweron();

	buf = (char *)kmalloc(4096, GFP_KERNEL);
	if (buf == NULL) {
		dump_debug("eeprom_read_boot: MALLOC FAIL!!");
		return;
	}

	addr = 0x0;
	for (j = 0; j < 1; j++) {	/* read 4K */
		len = 4096;
		do
			{
			err = wimax_i2c_read(addr, buf, len);
			}
		while(err<0?((retry--)>0):0);
		{	/* dump boot data */
			char print_buf[256] = {0};
			char chr[8] = {0};

			/* dump packets with checksum */
			u_char  *b = (u_char *)buf;
			dump_debug("EEPROM dump [0x%04x] = ", addr);

			for (i = 0; i < len; i++) {
				checksum += b[i];
				sprintf(chr, " %02x", b[i]);
				strcat(print_buf, chr);
				if (((i + 1) != len) && (i % 16 == 15)) {
					dump_debug(print_buf);
					memset(print_buf, 0x0, 256);
				}
			}
			dump_debug(print_buf);
		}
		addr += len;
	}

	dump_debug("Checksum: 0x%08x", checksum);

	kfree(buf);

	eeprom_poweroff();
	if(retry<0)
		dump_debug("eeprom error");
}

void eeprom_read_all()
{
	char	*buf = NULL;
	int	i, j;
	int	len;
	u_int	checksum = 0;
	short	addr = 0;
	int retry = 1000;
	int err;

	eeprom_poweron();

	/* allocate 4K buffer */
	buf = (char *)kmalloc(4096, GFP_KERNEL);
	if (buf == NULL) {
		dump_debug("eeprom_read_all: MALLOC FAIL!!");
		return;
	}

	addr = 0x0;
	for (j = 0; j < 16; j++)	/* read 64K */
	{
		len = 4096;
		do
			{
			err = wimax_i2c_read(addr, buf, len);
			}
		while(err<0?((retry--)>0):0);
		{	/* dump EEPROM */
			char print_buf[256] = {0};
			char chr[8] = {0};

			/* dump packets with checksum */
			u_char  *b = (u_char *)buf;
			dump_debug("EEPROM dump [0x%04x] = ", addr);

			for (i = 0; i < len; i++) {
				checksum += b[i];
				sprintf(chr, " %02x", b[i]);
				strcat(print_buf, chr);
				if (((i + 1) != len) && (i % 16 == 15)) {
					dump_debug(print_buf);
					memset(print_buf, 0x0, 256);
				}
			}
			dump_debug(print_buf);
		}
		addr += len;
	}

	dump_debug("Checksum: 0x%08x", checksum);

	/* free 4K buffer */
	kfree(buf);

	eeprom_poweroff();
	if(retry<0)
		dump_debug("eeprom error");
}

void eeprom_erase_all()
{
	char	buf[128] = {0};
	int	i;
	int retry = 1000;
	int err;

	eeprom_poweron();

	memset(buf, 0xff, 128);
	for (i = 0; i < 512; i++) {	/* clear all EEPROM */
		dump_debug("ERASE [0x%04x]\n", i * 128);
		do
			{
			err = wimax_i2c_write(128 * i, buf, 128);
			}
		while(err<0?((retry--)>0):0);
	}

	eeprom_poweroff();
	if(retry<0)
		dump_debug("eeprom error");
}

/* Write boot image to EEPROM */
int eeprom_write_boot(void)
{
	u_char	*buffer;
	int retry = 1000;
	int err = 0;
	u_short	ucsize = MAX_BOOT_WRITE_LENGTH;

	if (load_wimax_boot() < 0) {
		dump_debug("ERROR READ WIMAX BOOT IMAGE");
		return 0;
	}

	eeprom_poweron();

	g_wimax_image.offset = 0;

	while (g_wimax_image.size > g_wimax_image.offset) {
		buffer =(u_char *)(g_wimax_image.data + g_wimax_image.offset);
		ucsize = MAX_BOOT_WRITE_LENGTH;

		/* write buffer */
		do
			{
			err = wimax_i2c_write((u_short)g_wimax_image.offset, buffer, ucsize);
			}
		while(err<0?((retry--)>0):0);

		g_wimax_image.offset += MAX_BOOT_WRITE_LENGTH;

		if ((g_wimax_image.size - g_wimax_image.offset) < MAX_BOOT_WRITE_LENGTH) {
			buffer = (u_char *)(g_wimax_image.data + g_wimax_image.offset);
			ucsize = g_wimax_image.size - g_wimax_image.offset;

			/* write last data */
			do
				{
				err = wimax_i2c_write((u_short)g_wimax_image.offset, buffer, ucsize);
				}
			while(err<0?((retry--)>0):0);

			g_wimax_image.offset += MAX_BOOT_WRITE_LENGTH;
		}
	}

	eeprom_poweroff();

	if (g_wimax_image.data) {
		dump_debug("Delete the Image Loaded");
		vfree(g_wimax_image.data);
		g_wimax_image.data = NULL;
	}
	if(retry<0)
		{
		dump_debug("eeprom error");
	}else{
	dump_debug("EEPROM WRITING DONE.");
}
	return err;
}

int eeprom_write_rev(void)
{
	int ret;

	eeprom_poweron();
	ret = write_rev();	/* write hw rev to eeprom */
	eeprom_poweroff();

	dump_debug("REV WRITING DONE.");
	return ret;
}

void eeprom_erase_cert(void)
{
	eeprom_poweron();
	erase_cert();
	eeprom_poweroff();

	dump_debug("ERASE CERT DONE.");
}

int eeprom_check_cert(void)
{
	int	ret;

	eeprom_poweron();
	ret = check_cert();	/* check cert */
	eeprom_poweroff();

	dump_debug("CHECK CERT DONE. (ret: %d)", ret);

	return ret;
}

int eeprom_check_cal(void)
{
	int	ret;

	eeprom_poweron();
	ret = check_cal();	/* check cal */
	eeprom_poweroff();

	dump_debug("CHECK CAL DONE. (ret: %d)", ret);

	return ret;
}
