/* set chip configuration here for the config tool */
#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
	uint16_t vid;
	uint16_t pid;
} vid_pid_t;

#define SPI_SETTINGS "spi_settings"
#define CHIP_SETTINGS "chip_settings"
#define PRODUCT_NAME "product_name"
#define MANUFACTURER_NAME "manufacturer_name"
#define KEY_PARAMS "key_parameters"

// SPI settings
mcp2210_spi_settings_t config_spi_settings =
{
	.bitrate               = b32(1000000),
	.idle_cs               = b16(0),
	.active_cs             = b16(0x1ff),
	.cs_to_data_delay      = b16(0),
	.data_to_cs_delay      = b16(0),
	.data_delay            = b16(0),
	.bytes_per_transaction = b16(4),
	.spi_mode              = 0
};

// Chip settings
mcp2210_chip_settings_t config_chip_settings =
{
	.pins = { MCP2210_PIN_GPIO,
			MCP2210_PIN_CS,
			MCP2210_PIN_DEDICATED,
			MCP2210_PIN_DEDICATED,
			MCP2210_PIN_DEDICATED,
			MCP2210_PIN_DEDICATED,
			MCP2210_PIN_DEDICATED,
			MCP2210_PIN_GPIO,
			MCP2210_PIN_GPIO },
	.gpio_default   = b16(0x1ff),
	.gpio_direction = b16(0x1ff),
	.other_settings = 0x10,
	.nvram_lock     = 0,
	.new_password   = { 0, 0, 0, 0, 0, 0, 0, 0 }
};

// Key parameters
mcp2210_key_parameters_t config_key_parameters =
{
	.vid            = b16(MCP2210_VID),
	.pid            = b16(MCP2210_PID),
	//.power_options = SELF_POWERED,
	.power_options  = HOST_POWERED,
	// Required current in 2 mA
	.current_amount = 50
};

// Product name
#define CONFIG_PRODUCT_NAME "MCP2210 USB to SPI Master"
// Manufacturer name
#define CONFIG_MANUFACTURER_NAME "Microchip Technology Inc."

#endif
