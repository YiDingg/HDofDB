#include "SHT3X.h"

/**
 * @brief Send a 16-bit command to the SHT3X
 * @param cmd SHT3X command (defined in enum SHT3X_MODE)
 * @retval Status of the operation
 */
HAL_StatusTypeDef Send_Cmd(SHT3X_CMD cmd) {
    uint8_t cmd_buffer[2] = {cmd >> 8, cmd};
    return HAL_I2C_Master_Transmit(
        &hi2c1, SHT3X_ADDRESS_WRITE, (uint8_t*)cmd_buffer, 2, 0xFFFF);
}

/**
 * @brief	Reset SHT3X
 * @param	none
 * @retval	none
 */
void Reset(void) {
    Send_Cmd(SOFT_RESET_CMD);
    HAL_Delay(20);
}

/**
 * @brief	Get data from STH3X (once mode)
 * @param	dat the address of your varible to restore the data (6-byte array)
 * @retval	Status of the operation
 */
HAL_StatusTypeDef OnceMode_GetData(uint8_t* dat) {
    HAL_StatusTypeDef Status = Send_Cmd(HIGH_ENABLED_CMD);
    HAL_Delay(20);
    if (Status != HAL_OK) { return Status; }
    return HAL_I2C_Master_Receive(&hi2c1, SHT3X_ADDRESS_READ, dat, 6, 0xFFFF);
    /* return HAL_I2C_Mem_Read(&hi2c1, SHT3X_ADDRESS_READ, 0x00,
     * I2C_MEMADD_SIZE_8BIT, dat, 6, 0xFFFF);*/
}

/**
 * @brief	Enable SHT3X period mode
 * @param	none
 * @retval	Status of the operation
 */
HAL_StatusTypeDef PeriodMode_Enable(void) {
    return Send_Cmd(MEDIUM_2_CMD);
}

/**
 * @brief	Read data from SHT3X (period mode)
 * @param	dat the address of your varible to restore the data (6-byte array)
 * @retval	Status of the operation
 */
HAL_StatusTypeDef PeriodMode_GetData(uint8_t* dat) {
    Send_Cmd(READOUT_FOR_PERIODIC_MODE);
    return HAL_I2C_Master_Receive(&hi2c1, SHT3X_ADDRESS_READ, dat, 6, 0xFFFF);
}

/**
 * @brief	Convert raw data to temperature
 * @param	dat the address of the varible that restored the data (6-byte array)
 * @retval	0: success
 * @retval	1: failure
 */
uint8_t Data_To_Float(const uint8_t* dat, float* temp, float* humi) {
    uint16_t recv_temperature = 0;
    uint16_t recv_humidity = 0;

    /* 校验温度数据和湿度数据是否接收正确 */
    if (CheckCrc8(dat, 0xFF) != dat[2] || CheckCrc8(&dat[3], 0xFF) != dat[5]) {
        return 1;
    }
    /* 转换温度数据 */
    recv_temperature = ((uint16_t)dat[0] << 8) | dat[1];
    *temp = -45 + 175 * ((float)recv_temperature / 65535);
    /* 转换湿度数据 */
    recv_humidity = ((uint16_t)dat[3] << 8) | dat[4];
    *humi = 100 * ((float)recv_humidity / 65535);
    return 0;
}

#define CRC8_POLYNOMIAL 0x31

/**
 * @brief Calculate CRC8 code of a message
 * @param message
 * @param initial_value
 * @retval remainder:
 */
uint8_t CheckCrc8(const uint8_t* message, uint8_t initial_value) {
    uint8_t remainder;    // 余数
    uint8_t i = 0, j = 0; // 循环变量

    /* 初始化 */
    remainder = initial_value;

    for (j = 0; j < 2; j++) {
        remainder ^= message[j];

        /* 从最高位开始依次计算  */
        for (i = 0; i < 8; i++) {
            if (remainder & 0x80) {
                remainder = (remainder << 1) ^ CRC8_POLYNOMIAL;
            } else {
                remainder = (remainder << 1);
            }
        }
    }

    /* 返回计算的CRC码 */
    return remainder;
}

/* Structure for user's usage */
struct UserFuntions SHT3X = {
    OnceMode_GetData, PeriodMode_Enable, PeriodMode_GetData, Data_To_Float};
