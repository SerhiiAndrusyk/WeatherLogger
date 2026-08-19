#include "bme280.h"

#define BME280_REGISTER_ID           0xD0U
#define BME280_REGISTER_RESET        0xE0U
#define BME280_REGISTER_CTRL_HUM     0xF2U
#define BME280_REGISTER_STATUS       0xF3U
#define BME280_REGISTER_CTRL_MEAS    0xF4U
#define BME280_REGISTER_CONFIG       0xF5U
#define BME280_REGISTER_DATA         0xF7U

#define BME280_REGISTER_CALIB_TP     0x88U
#define BME280_REGISTER_CALIB_H1     0xA1U
#define BME280_REGISTER_CALIB_H2     0xE1U

#define BME280_CHIP_ID               0x60U
#define BME280_RESET_COMMAND         0xB6U

#define BME280_STATUS_IM_UPDATE      0x01U

#define BME280_CTRL_HUM_OVERSAMPLING_X1  0x01U
#define BME280_CTRL_MEAS_SLEEP_X1        0x24U
#define BME280_CONFIG_FILTER_OFF         0x00U

#define BME280_I2C_TIMEOUT_MS        1000U
#define BME280_RESET_TIMEOUT_MS      100U


static BME280_Status BME280_ReadRegisters(BME280_Handle *sensor, uint8_t registerAddress, uint8_t *data, uint16_t size);
static BME280_Status BME280_WriteRegister(BME280_Handle *sensor, uint8_t registerAddress, uint8_t data);
static BME280_Status BME280_WaitForCalibrationCopy(BME280_Handle *sensor);
static BME280_Status BME280_ReadCalibration(BME280_Handle *sensor);
static uint16_t BME280_ReadUInt16LE(const uint8_t *data);
static int16_t BME280_ReadInt16LE(const uint8_t *data);


static BME280_Status BME280_ReadRegisters(BME280_Handle *sensor, uint8_t registerAddress, uint8_t *data, uint16_t size){
    if ((sensor == NULL) ||
        (sensor->hi2c == NULL) ||
        (data == NULL) ||
        (size == 0U)){
        return BME280_INVALID_ARGUMENT;
    }

    HAL_StatusTypeDef halStatus = HAL_I2C_Mem_Read(
        sensor->hi2c,
        sensor->address,
        registerAddress,
        I2C_MEMADD_SIZE_8BIT,
        data,
        size,
        BME280_I2C_TIMEOUT_MS
    );

    if (halStatus == HAL_OK){
        return BME280_OK;
    }

    if (halStatus == HAL_TIMEOUT){
        return BME280_TIMEOUT;
    }

    return BME280_ERROR;
}

static BME280_Status BME280_WriteRegister(BME280_Handle *sensor, uint8_t registerAddress, uint8_t data){
    if ((sensor == NULL) || (sensor->hi2c == NULL)){
        return BME280_INVALID_ARGUMENT;
    }

    HAL_StatusTypeDef halStatus = HAL_I2C_Mem_Write(
        sensor->hi2c,
        sensor->address,
        registerAddress,
        I2C_MEMADD_SIZE_8BIT,
        &data,
        1U,
        BME280_I2C_TIMEOUT_MS
    );

    if (halStatus == HAL_OK){
        return BME280_OK;
    }

    if (halStatus == HAL_TIMEOUT){
        return BME280_TIMEOUT;
    }

    return BME280_ERROR;
}

static BME280_Status BME280_WaitForCalibrationCopy(BME280_Handle *sensor){
    uint32_t startTick = HAL_GetTick();
    uint8_t statusRegister;

    do{
        BME280_Status status = BME280_ReadRegisters(sensor, BME280_REGISTER_STATUS, &statusRegister, 1U);

        if (status != BME280_OK){
            return status;
        }

        if ((statusRegister & BME280_STATUS_IM_UPDATE) == 0U){
            return BME280_OK;
        }
    } while ((HAL_GetTick() - startTick) < BME280_RESET_TIMEOUT_MS);

    return BME280_TIMEOUT;
}

static uint16_t BME280_ReadUInt16LE(const uint8_t *data){
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static int16_t BME280_ReadInt16LE(const uint8_t *data){
    return (int16_t)BME280_ReadUInt16LE(data);
}

static BME280_Status BME280_ReadCalibration(BME280_Handle *sensor){
    uint8_t calibrationTP[26];
    uint8_t calibrationH[7];

    BME280_Status status = BME280_ReadRegisters(
        sensor,
        BME280_REGISTER_CALIB_TP,
        calibrationTP,
        sizeof(calibrationTP)
    );

    if (status != BME280_OK){
        return status;
    }

    status = BME280_ReadRegisters(
        sensor,
        BME280_REGISTER_CALIB_H2,
        calibrationH,
        sizeof(calibrationH)
    );

    if (status != BME280_OK){
        return status;
    }

    sensor->digT1 = BME280_ReadUInt16LE(&calibrationTP[0]);
    sensor->digT2 = BME280_ReadInt16LE(&calibrationTP[2]);
    sensor->digT3 = BME280_ReadInt16LE(&calibrationTP[4]);

    sensor->digP1 = BME280_ReadUInt16LE(&calibrationTP[6]);
    sensor->digP2 = BME280_ReadInt16LE(&calibrationTP[8]);
    sensor->digP3 = BME280_ReadInt16LE(&calibrationTP[10]);
    sensor->digP4 = BME280_ReadInt16LE(&calibrationTP[12]);
    sensor->digP5 = BME280_ReadInt16LE(&calibrationTP[14]);
    sensor->digP6 = BME280_ReadInt16LE(&calibrationTP[16]);
    sensor->digP7 = BME280_ReadInt16LE(&calibrationTP[18]);
    sensor->digP8 = BME280_ReadInt16LE(&calibrationTP[20]);
    sensor->digP9 = BME280_ReadInt16LE(&calibrationTP[22]);

    sensor->digH1 = calibrationTP[25];
    sensor->digH2 = BME280_ReadInt16LE(&calibrationH[0]);
    sensor->digH3 = calibrationH[2];

    uint16_t digH4 = ((uint16_t)calibrationH[3] << 4U) | (calibrationH[4] & 0x0FU);
    uint16_t digH5 = ((uint16_t)calibrationH[5] << 4U) | (calibrationH[4] >> 4U);

    if ((digH4 & 0x0800U) != 0U){
        digH4 |= 0xF000U;
    }

    if ((digH5 & 0x0800U) != 0U){
        digH5 |= 0xF000U;
    }

    sensor->digH4 = (int16_t)digH4;
    sensor->digH5 = (int16_t)digH5;
    sensor->digH6 = (int8_t)calibrationH[6];

    return BME280_OK;
}

BME280_Status BME280_Init(BME280_Handle *sensor, I2C_HandleTypeDef *hi2c, uint16_t address){
    if ((sensor == NULL) || (hi2c == NULL)){
        return BME280_INVALID_ARGUMENT;
    }

    sensor->hi2c = hi2c;
    sensor->address = address;

    uint8_t chipId;
    BME280_Status status = BME280_ReadRegisters(sensor, BME280_REGISTER_ID, &chipId, 1U);

    if (status != BME280_OK){
        return status;
    }

    if (chipId != BME280_CHIP_ID){
        return BME280_ERROR;
    }

    status = BME280_WriteRegister(sensor, BME280_REGISTER_RESET, BME280_RESET_COMMAND);

    if (status != BME280_OK){
        return status;
    }

    HAL_Delay(2U);

    status = BME280_WaitForCalibrationCopy(sensor);

    if (status != BME280_OK){
        return status;
    }

    status = BME280_ReadCalibration(sensor);

    if (status != BME280_OK){
        return status;
    }

    status = BME280_WriteRegister(sensor, BME280_REGISTER_CTRL_HUM, BME280_CTRL_HUM_OVERSAMPLING_X1);

    if (status != BME280_OK){
        return status;
    }

    status = BME280_WriteRegister(sensor, BME280_REGISTER_CONFIG, BME280_CONFIG_FILTER_OFF);

    if (status != BME280_OK){
        return status;
    }

    status = BME280_WriteRegister(sensor, BME280_REGISTER_CTRL_MEAS, BME280_CTRL_MEAS_SLEEP_X1);

    if (status != BME280_OK){
        return status;
    }

    return BME280_OK;
}
