#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/skbuff.h>
#include <linux/wimax/samsung/wimax732.h>

#include <plat/devs.h>
#include <plat/sdhci.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>

#if defined(CONFIG_WIMAX_CMC)
#define dump_debug(args...)				\
	{						\
		printk(KERN_ALERT"\x1b[1;33m[WiMAX] "); \
		printk(args);				\
		printk("\x1b[0m\n");			\
	}

extern int s3c_bat_use_wimax(int onoff);

static struct wimax_cfg wimax_config;

void wimax_on_pin_conf(int onoff)
{
	int gpio;
	dump_debug("[WIMAX] wimax_on_pin_conf");
	if (onoff) {
		dump_debug("[WIMAX] wimax_on_pin_conf - onoff");
		for (gpio = EXYNOS4_GPK3(0); gpio < EXYNOS4_GPK3(2); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
		}

		for (gpio = EXYNOS4_GPK3(3); gpio <= EXYNOS4_GPK3(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
		}

	} else {
		dump_debug("[WIMAX] wimax_on_pin_conf - else onoff");
		for (gpio = EXYNOS4_GPK3(0); gpio < EXYNOS4_GPK3(2); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}
		for (gpio = EXYNOS4_GPK3(3); gpio <= EXYNOS4_GPK3(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}

	}

}

/*static void store_uart_path(void)
{
	wimax_config.uart_sel = gpio_get_value(GPIO_UART_SEL);
	wimax_config.uart_sel1 = gpio_get_value(GPIO_UART_SEL1);
}*/

static void wimax_deinit_gpios(void);
static void wimax_wakeup_assert(int enable)
{
	gpio_set_value(GPIO_WIMAX_WAKEUP, !enable);
}

static int get_wimax_sleep_mode(void)
{
	return gpio_get_value(GPIO_WIMAX_IF_MODE1);
}

static int is_wimax_active(void)
{
	return gpio_get_value(GPIO_WIMAX_CON0);
}

static void signal_ap_active(int enable)
{
	gpio_set_value(GPIO_WIMAX_CON1, enable);
}

static void switch_eeprom_wimax(void)
{
	dump_debug("[WIMAX] switch_eeprom_wimax");
	gpio_set_value(GPIO_WIMAX_I2C_CON, 0);
	msleep(10);
}

static void switch_usb_ap(void)
{
	dump_debug("[WIMAX] switch_usb_ap");
	gpio_set_value(GPIO_WIMAX_USB_EN, 0);
	msleep(10);
}

static void switch_usb_wimax(void)
{
	dump_debug("[WIMAX] switch_usb_wimax");
	gpio_set_value(GPIO_WIMAX_USB_EN, 1);
	msleep(10);
}

static void switch_uart_wimax(void)
{
	dump_debug("[WIMAX] switch_uart_wimax");
	gpio_set_value(GPIO_UART_SEL, 0);
	gpio_set_value(GPIO_UART_SEL1, 1);
}

static void wimax_init_gpios(void)
{
	dump_debug("[WIMAX] wimax_init_gpios");
	s3c_gpio_cfgpin(GPIO_WIMAX_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_WIMAX_INT, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_WIMAX_CON0, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_WIMAX_CON0, S3C_GPIO_PULL_UP);
	gpio_set_value(GPIO_WIMAX_IF_MODE1, 1);	/* default idle */
	gpio_set_value(GPIO_WIMAX_CON2, 1);	/* active low interrupt */
	gpio_set_value(GPIO_WIMAX_CON1, 1);
	
	signal_ap_active(1);
}

static void hw_set_wimax_mode(void)
{
      dump_debug("[WIMAX] hw_set_wimax_mode");
	switch (wimax_config.wimax_mode) {
	case SDIO_MODE:
		pr_debug("SDIO MODE");
		gpio_set_value(GPIO_WIMAX_WAKEUP, 1);
		gpio_set_value(GPIO_WIMAX_IF_MODE0, 1);
		break;
	case WTM_MODE:
	case AUTH_MODE:
		pr_debug("WTM_MODE || AUTH_MODE");
		gpio_set_value(GPIO_WIMAX_WAKEUP, 0);
		gpio_set_value(GPIO_WIMAX_IF_MODE0, 0);
		break;
	case DM_MODE:
		pr_debug("DM MODE");
		gpio_set_value(GPIO_WIMAX_WAKEUP, 1);
		gpio_set_value(GPIO_WIMAX_IF_MODE0, 0);
		break;
	case USB_MODE:
	case USIM_RELAY_MODE:
		pr_debug("USB MODE");
		gpio_set_value(GPIO_WIMAX_WAKEUP, 0);
		gpio_set_value(GPIO_WIMAX_IF_MODE0, 1);
		break;
	}
}

static void wimax_hsmmc_presence_check(void)
{
	sdhci_s3c_force_presence_change(&s3c_device_hsmmc3);
}

/*static void display_gpios(void)
{
	int val = 0;
	val = gpio_get_value(GPIO_WIMAX_EN);
	dump_debug("WIMAX_EN: %d", val);
	val = gpio_get_value(GPIO_WIMAX_RESET_N);
	dump_debug("WIMAX_RESET: %d", val);
	val = gpio_get_value(GPIO_WIMAX_CON0);
	dump_debug("WIMAX_CON0: %d", val);
	val = gpio_get_value(GPIO_WIMAX_CON1);
	dump_debug("WIMAX_CON1: %d", val);
	val = gpio_get_value(GPIO_WIMAX_CON2);
	dump_debug("WIMAX_CON2: %d", val);
	val = gpio_get_value(GPIO_WIMAX_WAKEUP);
	dump_debug("WIMAX_WAKEUP: %d", val);
	val = gpio_get_value(GPIO_WIMAX_INT);
	dump_debug("WIMAX_INT: %d", val);
	val = gpio_get_value(GPIO_WIMAX_IF_MODE0);
	dump_debug("WIMAX_IF_MODE0: %d", val);
	val = gpio_get_value(GPIO_WIMAX_IF_MODE1);
	dump_debug("WIMAX_IF_MODE1: %d", val);
	val = gpio_get_value(GPIO_WIMAX_USB_EN);
	dump_debug("WIMAX_USB_EN: %d", val);
	val = gpio_get_value(GPIO_WIMAX_I2C_CON);
	dump_debug("I2C_SEL: %d", val);
	val = gpio_get_value(GPIO_UART_SEL);
	dump_debug("UART_SEL1: %d", val);
}*/

static int gpio_wimax_power(int enable)
{
	int try_count = 0;

	if (!enable)
		goto wimax_power_off;
	if (gpio_get_value(GPIO_WIMAX_EN)) {
		dump_debug("Already Wimax powered ON");
		return WIMAX_ALREADY_POWER_ON;
	}
	while (!wimax_config.card_removed) {
		try_count++;
		msleep(100);
		if (try_count == 50)
			break;
	}
	dump_debug("Wimax power ON");
	if (wimax_config.wimax_mode != SDIO_MODE) {
	      dump_debug("Wimax power ON - !SDIO_MODE");
		switch_usb_wimax();
		//s3c_bat_use_wimax(1);
	}
	if (wimax_config.wimax_mode == WTM_MODE) {
		dump_debug("Wimax power ON - !WTM_MODE");
		//store_uart_path();
		switch_uart_wimax();
	}
	switch_eeprom_wimax();
	dump_debug("Wimax power ON - switch_eeprom_wimax");
	wimax_on_pin_conf(1);
	dump_debug("Wimax power ON - wimax_on_pin_conf");
	gpio_set_value(GPIO_WIMAX_EN, 1);
	dump_debug("Wimax power ON - gpio_set_value(GPIO_WIMAX_EN, GPIO_LEVEL_HIGH)");
	wimax_init_gpios();
	dump_debug("Wimax power ON - wimax_init_gpios()");
	dump_debug("RESET");
	gpio_set_value(GPIO_WIMAX_RESET_N, 0);
	dump_debug("Wimax power ON - gpio_set_value(GPIO_WIMAX_RESET_N, GPIO_LEVEL_LOW);");
	mdelay(3);
	dump_debug("Wimax power ON - mdelay(3)");	
	gpio_set_value(GPIO_WIMAX_RESET_N, 1);
	msleep(400);
	dump_debug("Wimax power ON - msleep(400)");
	if (wimax_config.card_removed){
		wimax_hsmmc_presence_check();
		dump_debug("Wimax power ON - wimax_hsmmc_presence_check()");
	}
	return WIMAX_POWER_SUCCESS;
wimax_power_off:
	/*Wait for modem to flush EEPROM data*/
	//mutex_lock(&wimax_config.poweroff_mutex);
	pr_debug("Wimax power OFF - SBRISSEN");
	wimax_on_pin_conf(0);
	pr_debug("Wimax power OFF - SBRISSEN - wimax_on_pin_conf");
	msleep(500);
	wimax_deinit_gpios();

	pr_debug("Wimax power OFF");

	/*Dont force detect if the card is already detected as removed*/
	if (!wimax_config.card_removed){
	      pr_debug("Wimax power OFF-hsmmc-presence-check");
		wimax_hsmmc_presence_check();
	}

	/*Not critial, just some safty margin*/
	msleep(300);
	//mutex_unlock(&wimax_config.poweroff_mutex);
	return WIMAX_POWER_SUCCESS;
}



/*static void restore_uart_path(void)
{
	gpio_set_value(GPIO_UART_SEL, wimax_config.uart_sel);
	gpio_set_value(GPIO_UART_SEL1, wimax_config.uart_sel1);
}*/

/*static void switch_uart_ap(void)
{
	gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);
}*/

static struct wimax732_platform_data wimax732_pdata = {
	.power = gpio_wimax_power,
	.set_mode = hw_set_wimax_mode,
	.signal_ap_active = signal_ap_active,
	.get_sleep_mode = get_wimax_sleep_mode,
	.is_modem_awake = is_wimax_active,
	.wakeup_assert = wimax_wakeup_assert,
	//.uart_wimax = switch_uart_wimax,
	//.uart_ap = switch_uart_ap,
	//.gpio_display = display_gpios,
	//.restore_uart_path = restore_uart_path,
	.g_cfg = &wimax_config,
	//.wimax_int	= GPIO_USB_SEL,
};

struct platform_device s3c_device_cmc732 = {
	.name = "wimax732_driver",
	.id = 1,
	.dev.platform_data = &wimax732_pdata,
};

void wimax_deinit_gpios(void)
{
	dump_debug("wimax_deinit_gpios");
	s3c_gpio_cfgpin(GPIO_WIMAX_DBGEN_28V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_DBGEN_28V, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_DBGEN_28V, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_I2C_CON, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_I2C_CON, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_I2C_CON, GPIO_LEVEL_HIGH);

	s3c_gpio_cfgpin(GPIO_WIMAX_WAKEUP, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_WAKEUP, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_WAKEUP, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_IF_MODE0, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_IF_MODE0, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_IF_MODE0, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_CON0, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_CON0, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_CON0, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_IF_MODE1, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_IF_MODE1, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_IF_MODE1, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_CON2, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_CON2, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_CON2, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_CON1, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_CON1, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_CON1, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_RESET_N, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_RESET_N, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_RESET_N, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_EN, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_USB_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_USB_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_USB_EN, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_UART_SEL1, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_CMC_SCL_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_CMC_SCL_18V, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CMC_SCL_18V, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_CMC_SDA_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_CMC_SDA_18V, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CMC_SDA_18V, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_WIMAX_INT, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_INT, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_INT, GPIO_LEVEL_LOW);

	switch_usb_ap();
	
	//wimax732_pdata.wimax_int = GPIO_USB_SEL;
}

#endif














