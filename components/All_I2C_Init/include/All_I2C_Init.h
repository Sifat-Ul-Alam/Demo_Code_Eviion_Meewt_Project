#include "stdint.h"
#include "driver/i2c_master.h"


void      I2C_Bus_Init(void);
void      I2C_Scan(void);

esp_err_t QMA6100P_Write_Reg(uint8_t reg_addr, uint8_t data);
esp_err_t QMA6100P_Read_Regs(uint8_t reg_addr, uint8_t *data_buf, size_t len);

void      I2C_Bus_Deinit(void);