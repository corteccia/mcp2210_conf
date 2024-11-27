/* MCP2210 configuration tool */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include "hid.h"
#include "mcp2210.h"
#include "config.h"

static ssize_t get_device_list(hid_handle_t ***devices,
							   uint16_t vid,
							   uint16_t pid)
{
	ssize_t device_count;
	for (size_t i = 10;; i *= 2) {
		*devices = malloc(i);
		if (!*devices) abort();
		device_count = hid_find_devices(vid, pid, *devices, 10);
		if (-1 == device_count) {
			if (errno == ENOMEM) {
				free(*devices);
				continue;
			}
			return -1;
		}
		break;
	}
	return device_count;
}

static int command_list(vid_pid_t device_vp)
{
	hid_handle_t **devices;
	ssize_t device_count = get_device_list(&devices, device_vp.vid, device_vp.pid);
	if (-1 == device_count) {
		perror("Failed to search for HID devices");
		return 1;
	}

	if (!device_count) {
		printf("Found no devices\n");
		goto done;
	}

	printf("Found %ld devices\n", device_count);
	for (size_t i = 0; i < device_count; ++i) {
		printf("[%ld] %s\n", i + 1, hid_device_desc(devices[i]));
		hid_cleanup_device(devices[i]);
	}

done:
	free(devices);
	return 0;
}

static void print_spi_settings(mcp2210_spi_settings_t s)
{
	printf("{\n\t.bitrate = %d\n\t.idle_cs = 0x%04x\n"
		"\t.active_cs = 0x%04x\n\t.cs_to_data_delay = 0x%04x\n"
		"\t.data_to_cs_delay = 0x%04x\n\t.data_delay = 0x%04x\n"
		"\t.bytes_per_transaction = 0x%04x\n"
		"\t.spi_mode = %d\n}\n",
		 b32(s.bitrate), b16(s.idle_cs), b16(s.active_cs), b16(s.cs_to_data_delay),
		 b16(s.data_to_cs_delay), b16(s.data_delay), b16(s.bytes_per_transaction),
		 s.spi_mode);
}

static void print_chip_settings(mcp2210_chip_settings_t s)
{
	printf("{\n\t.pins = \n\t{\n");
	for (size_t i = 0; i < 9; ++i) {
		uint8_t pin = s.pins[i];
		switch (pin) {
		case MCP2210_PIN_GPIO:
			printf("\t\tMCP2210_PIN_GPIO,\n");
			break;
		case MCP2210_PIN_CS:
			printf("\t\tMCP2210_PIN_CS,\n");
			break;
		case MCP2210_PIN_DEDICATED:
			printf("\t\tMCP2210_PIN_DEDICATED,\n");
			break;
		}
	}
	printf("\t},\n\t.gpio_default = 0x%04x,\n\t.gpio_direction = 0x%04x,\n"
		"\t.other_settings = 0x%02x,\n\t.nvram_lock = 0x%02x\n}\n",
		b16(s.gpio_default), b16(s.gpio_direction), s.other_settings,
		s.nvram_lock);
}

static void print_key_parameters(mcp2210_key_parameters_t s)
{
	printf("{\n\t.vid = 0x%04x\n\t.pid = 0x%04x\n"
		"\t.power_options = 0x%02x\n\t.current_amount = %d mA\n}\n",
		 b16(s.vid), b16(s.pid), s.power_options, s.current_amount * 2);
}

static int get_set_product_name(hid_handle_t *device, bool get, char *product_name)
{
	if (!get) {
		printf("Writing product name...\n");
		if (-1 == write_product_name(device, product_name)) {
			perror("Failed to write product name");
			return 1;
		}
	}

	printf("Reading product name...\n");
	char *buffer = malloc(100);
	if (-1 == read_product_name(device, buffer, 100)) {
		free(buffer);
		perror("Failed to read product name");
		return 1;
	}
	printf("Product name: %s\n", buffer);
	free(buffer);
	return 0;
}

static int get_set_manufacturer_name(hid_handle_t *device, bool get, char *manufacturer_name)
{
	if (!get) {
		printf("Writing manufacturer name...\n");
		if (-1 == write_manufacturer_name(device, manufacturer_name)) {
			perror("Failed to write manufacturer name");
			return 1;
		}
	}

	printf("Reading manufacturer name...\n");
	char *buffer = malloc(100);
	if (-1 == read_manufacturer_name(device, buffer, 100)) {
		free(buffer);
		perror("Failed to read manufacturer name");
		return 1;
	}
	printf("Manufacturer name: %s\n", buffer);
	free(buffer);
	return 0;
}

static int get_set_key_parameters(hid_handle_t *device, bool get, mcp2210_key_parameters_t *new_key_params)
{
	if (!get) {
		printf("Writing key parameters...\n");
		if (-1 == write_key_parameters(device, new_key_params)) {
			perror("Failed to write key parameters");
			return 1;
		}
	}

	printf("Reading key parameters...\n");
	mcp2210_key_parameters_t key_parameters;
	if (-1 == read_key_parameters(device, &key_parameters)) {
		perror("Failed to read key parameters");
		return 1;
	}
	print_key_parameters(key_parameters);
	return 0;
}

static int command_get_set(vid_pid_t device_vp, bool get, char *settings, int index, bool is_nvram)
{
	hid_handle_t **devices;
	ssize_t device_count = get_device_list(&devices, device_vp.vid, device_vp.pid);
	if (-1 == device_count) {
		perror("Failed to search for HID devices");
		return 1;
	}

	int status = 0;

	if (index > device_count || !index) {
		fprintf(stderr, "Invalid device number %d\n", index);
		status = 1;
		goto done;
	}

	if (!strcmp(settings, SPI_SETTINGS)) {
		mcp2210_spi_settings_t spi_settings;

		if (!get) {
			printf("Writing SPI settings...\n");
			if (-1 == write_spi_settings(devices[index - 1], &config_spi_settings, is_nvram)) {
				perror("Failed to write SPI settings");
				status = 1;
				goto done;
			}
			printf("Done\n");
		}

		printf("Reading SPI settings...\n");
		if (-1 == read_spi_settings(devices[index - 1], &spi_settings, is_nvram)) {
			perror("Failed to read SPI settings");
			status = 1;
			goto done;
		}
		print_spi_settings(spi_settings);
	} else if (!strcmp(settings, CHIP_SETTINGS)) {
		mcp2210_chip_settings_t chip_settings;

		if (!get) {
			printf("Writing chip settings...\n");
			if (-1 == write_chip_settings(devices[index - 1], &config_chip_settings, is_nvram)) {
				perror("Failed to write chip settings");
				status = 1;
				goto done;
			}
			printf("Done\n");
		}

		printf("Reading chip settings...\n");
		if (-1 == read_chip_settings(devices[index - 1], &chip_settings, is_nvram)) {
			perror("Failed to read chip settings");
			status = 1;
			goto done;
		}
		print_chip_settings(chip_settings);
	} else if (!strcmp(settings, PRODUCT_NAME)) {
		status = get_set_product_name(devices[index - 1], get, CONFIG_PRODUCT_NAME);
		if (status == 1)
			goto done;
	} else if (!strcmp(settings, MANUFACTURER_NAME)) {
		status = get_set_manufacturer_name(devices[index - 1], get, CONFIG_MANUFACTURER_NAME);
		if (status == 1)
			goto done;
	} else if (!strcmp(settings, KEY_PARAMS)) {
		status = get_set_key_parameters(devices[index - 1], get, &config_key_parameters);
		if (status == 1)
			goto done;
	} else {
		fprintf(stderr, "Unknown setting type %s\n", settings);
		status = 1;
		goto done;
	}

done:
	for (size_t i = 0; i < device_count; ++i) {
		hid_cleanup_device(devices[i]);
	}
	free(devices);
	return status;
}

static int command_config(vid_pid_t device_vp, char *config, int index)
{
	FILE * fp;
	uint8_t * p;

	fp = fopen(config, "r");
	if (fp == NULL) {
		perror("Opening config file failed");
		return 1;
	}

	hid_handle_t **devices;
	ssize_t device_count = get_device_list(&devices, device_vp.vid, device_vp.pid);
	if (-1 == device_count) {
		perror("Failed to search for HID devices");
		return 1;
	}

	int status = 0;

	if (index > device_count || !index) {
		fprintf(stderr, "Invalid device number %d\n", index);
		status = 1;
		goto done;
	}

	char line[256];
	int key_params_specified = false;
	mcp2210_key_parameters_t key_params = config_key_parameters;

	while (fgets(line, sizeof(line), fp)) {
		char *key = strtok(line, "=");
		char *value = strtok(NULL, "\n");

		if (!key || !value) continue;

		if (!strcmp(key, PRODUCT_NAME)) {
			status = get_set_product_name(devices[index - 1], false, value);
			if (status == 1)
				goto done;
		} else if (!strcmp(key, MANUFACTURER_NAME)) {
			status = get_set_manufacturer_name(devices[index - 1], false, value);
			if (status == 1)
				goto done;
		} else if (!strncmp(key, KEY_PARAMS ".", sizeof(KEY_PARAMS))) {
			key_params_specified = true;

			char *subkey = key + sizeof(KEY_PARAMS);
			if (!strcmp(subkey, "vid")) {
				key_params.vid = (unsigned short)strtol(value, NULL, 0);
			} else if (!strcmp(subkey, "pid")) {
				key_params.pid = (unsigned short)strtol(value, NULL, 0);
			}
		}
	}

	fclose(fp);

	if (key_params_specified) {
		status = get_set_key_parameters(devices[index - 1], false, &key_params);
		if (status == 1)
			goto done;
	}

done:
	for (size_t i = 0; i < device_count; ++i) {
		hid_cleanup_device(devices[i]);
	}
	free(devices);
	return status;
}

static void
mcp2210_usage (char * progname)
{
	fprintf (stderr, "%s: [-d custom vid:pid] [-c config_file]\n"
		"\t\t[-g get_settings] [-s set_settings] [-l]\n"
		"\nConfiguration of MCP2210 device\n"
		"\noptions:\n"
		"\t-d VID:PID provide custom vid:pid (default: 0x4d8:0x00de)\n"
		"\t-i INDEX   index of the device (default: 1)\n"
		"\t-c CONFIG  config file with parameters to set\n"
		"\t-g SETTING get the settings from the chip (spi_settings, chip_settings, etc.)\n"
		"\t-s SETTING set the settings from config (spi_settings, chip_settings, etc.)\n"
		"\t-n         set in NVRAM (for spi_settings and chip_settings)\n"
		"\t-l         list all the devices\n",
		progname
	);
	return;
}

int main(int argc, char **argv)
{
	int c;
	char * config = NULL;
	char * get = NULL;
	char * set = NULL;
	int index = 1;
	bool list = false;
	bool is_nvram = false;
	bool want_vid_pid = false;
	vid_pid_t device_vp = { 0, 0 };

	if (argc < 2) {
		mcp2210_usage (argv[0]);
		exit (0);
	}

	// Initialize HID library
	if (-1 == hid_init()) {
		perror("Failed to initialize HID module");
		return 1;
	}

	while ((c = getopt (argc, argv, "nlc:d:s:g:i:")) != -1) {
		switch(c) {
			case 'd':
				want_vid_pid = true;
				char *p = NULL;
				device_vp.vid = (uint16_t)strtol (optarg, &p, 16);
				if (*p != ':')
				{
					fprintf (stderr, "error: bad format to -d option, expected 0xXXXX:0xXXXX\n");
					exit(-1);
				}
				device_vp.pid = (uint16_t)strtol (++p, NULL, 16);
				break;
			case 'i':
				index = atoi(optarg);  // default = 1
				break;
			case 'g':
				get = optarg;
				break;
			case 's':
				set = optarg;
				break;
			case 'l':
				list = true;
				break;
			case 'c':
				config = optarg;
				break;
			case 'n':
				is_nvram = true;
				break;
			default:
				mcp2210_usage (argv[0]);
				exit (0);
				break;
		}
	}

	if (!want_vid_pid) {
		device_vp.vid = MCP2210_VID;
		device_vp.pid = MCP2210_PID;
	}

	if (list) {
		return command_list(device_vp);
	} else if (get != NULL) {
		return command_get_set(device_vp, true, get, index, is_nvram);
	} else if (set != NULL) {
		return command_get_set(device_vp, false, set, index, is_nvram);
	} else if (config != NULL) {
		return command_config(device_vp, config, index);
	}

	hid_fini();
	return 0;
}
