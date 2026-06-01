// SPDX-License-Identifier: GPL-2.0+
/*
 * cmd/panel.c - Symphony front-panel control (LED + 3 buttons)
 *
 * Drives the user LED (D10) and reads the three pushbuttons
 * (BACK/HOME/MENU) on the Variscite Symphony carrier, via the
 * PCA9534 GPIO expander described in DT under /symphony-panel.
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>

struct panel_gpios {
	struct gpio_desc led;
	struct gpio_desc back;
	struct gpio_desc home;
	struct gpio_desc menu;
	bool inited;
};

static struct panel_gpios panel;

static int panel_init(void)
{
	ofnode node;
	int ret;

	if (panel.inited)
		return 0;

	node = ofnode_path("/symphony-panel");
	if (!ofnode_valid(node)) {
		printf("panel: /symphony-panel not found in device tree\n");
		return -ENOENT;
	}

	ret = gpio_request_by_name_nodev(node, "led-gpios",  0,
					 &panel.led,  GPIOD_IS_OUT);
	if (ret) {
		printf("panel: cannot request led-gpios (%d)\n", ret);
		return ret;
	}
	ret = gpio_request_by_name_nodev(node, "back-gpios", 0,
					 &panel.back, GPIOD_IS_IN);
	if (ret) {
		printf("panel: cannot request back-gpios (%d)\n", ret);
		return ret;
	}
	ret = gpio_request_by_name_nodev(node, "home-gpios", 0,
					 &panel.home, GPIOD_IS_IN);
	if (ret) {
		printf("panel: cannot request home-gpios (%d)\n", ret);
		return ret;
	}
	ret = gpio_request_by_name_nodev(node, "menu-gpios", 0,
					 &panel.menu, GPIOD_IS_IN);
	if (ret) {
		printf("panel: cannot request menu-gpios (%d)\n", ret);
		return ret;
	}

	panel.inited = true;
	return 0;
}

static int do_panel_led(int argc, char *const argv[])
{
	int i;

	if (argc < 1)
		return CMD_RET_USAGE;

	if (!strcmp(argv[0], "on")) {
		dm_gpio_set_value(&panel.led, 1);
	} else if (!strcmp(argv[0], "off")) {
		dm_gpio_set_value(&panel.led, 0);
	} else if (!strcmp(argv[0], "blink")) {
		for (i = 0; i < 6; i++) {
			dm_gpio_set_value(&panel.led, i & 1);
			mdelay(150);
		}
		dm_gpio_set_value(&panel.led, 0);
	} else {
		return CMD_RET_USAGE;
	}
	return CMD_RET_SUCCESS;
}

static int do_panel_buttons(void)
{
	int b = dm_gpio_get_value(&panel.back);
	int h = dm_gpio_get_value(&panel.home);
	int m = dm_gpio_get_value(&panel.menu);

	printf("BACK=%d  HOME=%d  MENU=%d\n", b, h, m);
	if (b || h || m)
		printf("pressed:%s%s%s\n",
		       b ? " BACK" : "",
		       h ? " HOME" : "",
		       m ? " MENU" : "");
	return CMD_RET_SUCCESS;
}

static int do_panel(struct cmd_tbl *cmdtp, int flag, int argc,
		    char *const argv[])
{
	int ret;

	ret = panel_init();
	if (ret)
		return CMD_RET_FAILURE;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "led"))
		return do_panel_led(argc - 2, argv + 2);
	if (!strcmp(argv[1], "buttons"))
		return do_panel_buttons();

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	panel, 4, 0, do_panel,
	"Symphony front-panel control",
	"led on|off|blink   - drive the user LED (D10)\n"
	"panel buttons         - read BACK/HOME/MENU state"
);
