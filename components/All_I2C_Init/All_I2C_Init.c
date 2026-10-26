#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "stdio.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "driver/i2c_master.h"
#include "All_I2C_Init.h"

#define QMA6100P_I2C_ADDR       0x12  

static const char *TAG = "I2C_BUS";

// initialize
i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t QMA_dev_handle;

void I2C_Bus_Init(void)
{
    // 1. Initialize the Hardware Bus
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0, 
        .scl_io_num = 47,
        .sda_io_num = 48,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    // 2. Add RTC (7-bit address 0x68)
    i2c_device_config_t QMA_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = QMA6100P_I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &QMA_cfg, &QMA_dev_handle));
    ESP_LOGI(TAG, "I2C Bus and Devices initialized successfully.");
}

void I2C_Bus_Deinit(void)
{
    // 1. Remove the device first
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(QMA_dev_handle));

    // 2. Then delete the bus itself
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));

    ESP_LOGI(TAG, "I2C Bus and Devices deinitialized.");
}


// Scans the bus for any responding devices
void I2C_Scan(void) {
    printf("Scanning I2C Bus...\n");
    for (int i = 3; i < 0x78; i++) {
        if (i2c_master_probe(bus_handle, i, -1) == ESP_OK) {
            printf("Found device at: 0x%02X\n", i);
        }
    }
}

/**
 * @brief Writes a single byte to a specific register on the QMA6100P.
 * * @param reg_addr Target register address.
 * @param data Data byte to write.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t QMA6100P_Write_Reg(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    
    // Transmit the target register address followed by the data byte
    esp_err_t err = i2c_master_transmit(QMA_dev_handle, write_buf, sizeof(write_buf), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Register Write Failed to 0x%02X: %s", reg_addr, esp_err_to_name(err));
    }
    return err;
}

/**
 * @brief Reads one or multiple sequential bytes from the QMA6100P.
 * * @param reg_addr Starting register address to read from.
 * @param data_buf Pointer to the buffer where received data will be stored.
 * @param len Number of bytes to read (e.g., 1 for single reg, 6 for full X/Y/Z data).
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t QMA6100P_Read_Regs(uint8_t reg_addr, uint8_t *data_buf, size_t len)
{
    if (data_buf == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Transmit the register address, then perform a repeated start to read the data bytes
    esp_err_t err = i2c_master_transmit_receive(QMA_dev_handle, &reg_addr, 1, data_buf, len, -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Register Read Failed from 0x%02X: %s", reg_addr, esp_err_to_name(err));
    }
    return err;
}


