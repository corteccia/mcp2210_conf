#!/bin/sh
# Shell script to easily configure a new device

# Change NVRAM (power-up default) settings 
./mcp2210_conf -n -s spi_settings
./mcp2210_conf -n -s chip_settings

# Change the settings in RAM too to avoid re-plugging the device
./mcp2210_conf -s spi_settings
./mcp2210_conf -s chip_settings

# Changing these is usually not required
#./mcp2210_conf -s key_parameters
#./mcp2210_conf -s product_name
#./mcp2210_conf -s manufacturer_name
