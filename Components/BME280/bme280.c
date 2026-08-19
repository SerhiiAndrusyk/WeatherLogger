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

#define BME280_STATUS_MEASURING          0x08U
#define BME280_CTRL_MEAS_FORCED_X1       0x25U
#define BME280_MEASUREMENT_TIMEOUT_MS    100U


static BME280_Status BME280_ReadRegisters(BME280_Handle *sensor, uint8_t registerAddress, uint8_t *data, uint16_t size);
static BME280_Status BME280_WriteRegister(BME280_Handle *sensor, uint8_t registerAddress, uint8_t data);
static BME280_Status BME280_WaitForCalibrationCopy(BME280_Handle *sensor);
static BME280_Status BME280_ReadCalibration(BME280_Handle *sensor);
static uint16_t BME280_ReadUInt16LE(const uint8_t *data);
static int16_t BME280_ReadInt16LE(const uint8_t *data);
static BME280_Status BME280_WaitForMeasurement(BME280_Handle *sensor);
static int32_t BME280_CompensateTemperature(BME280_Handle *sensor, int32_t rawTemperature);
static uint32_t BME280_CompensatePressure(BME280_Handle *sensor, int32_t rawPressure);
static uint32_t BME280_CompensateHumidity(BME280_Handle *sensor, int32_t rawHumidity);


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

static BME280_Status BME280_WaitForMeasurement(BME280_Handle *sensor){
    uint32_t startTick = HAL_GetTick();
    uint8_t statusRegister;

    do{
        BME280_Status status = BME280_ReadRegisters(
            sensor,
            BME280_REGISTER_STATUS,
            &statusRegister,
            1U
        );

        if (status != BME280_OK){
            return status;
        }

        if ((statusRegister & (BME280_STATUS_MEASURING | BME280_STATUS_IM_UPDATE)) == 0U){
            return BME280_OK;
        }
    } while ((HAL_GetTick() - startTick) < BME280_MEASUREMENT_TIMEOUT_MS);

    return BME280_TIMEOUT;
}

static int32_t BME280_CompensateTemperature(BME280_Handle *sensor, int32_t rawTemperature){
    int32_t difference =
        (rawTemperature >> 3) - ((int32_t)sensor->digT1 << 1);

    int32_t var1 =
        (difference * (int32_t)sensor->digT2) >> 11;

    difference =
        (rawTemperature >> 4) - (int32_t)sensor->digT1;

    int32_t var2 =
        (((difference * difference) >> 12) * (int32_t)sensor->digT3) >> 14;

    sensor->tFine = var1 + var2;

    return (sensor->tFine * 5 + 128) >> 8;
}

static uint32_t BME280_CompensatePressure(BME280_Handle *sensor, int32_t rawPressure){
    int64_t var1 = (int64_t)sensor->tFine - 128000;
    int64_t var2 = var1 * var1 * (int64_t)sensor->digP6;

    var2 += (var1 * (int64_t)sensor->digP5) << 17;
    var2 += (int64_t)sensor->digP4 << 35;

    var1 =
        ((var1 * var1 * (int64_t)sensor->digP3) >> 8) +
        ((var1 * (int64_t)sensor->digP2) << 12);

    var1 =
        (((((int64_t)1 << 47) + var1) *
        (int64_t)sensor->digP1) >> 33);

    if (var1 == 0){
        return 0U;
    }

    int64_t pressure = 1048576 - rawPressure;

    pressure =
        (((pressure << 31) - var2) * 3125) / var1;

    var1 =
        ((int64_t)sensor->digP9 *
        (pressure >> 13) *
        (pressure >> 13)) >> 25;

    var2 =
        ((int64_t)sensor->digP8 *
        pressure) >> 19;

    pressure =
        ((pressure + var1 + var2) >> 8) +
        ((int64_t)sensor->digP7 << 4);

    return (uint32_t)pressure;
}

static uint32_t BME280_CompensateHumidity(BME280_Handle *sensor, int32_t rawHumidity){
    int32_t humidity = sensor->tFine - 76800;

    humidity =
        (((((rawHumidity << 14) -
        ((int32_t)sensor->digH4 << 20) -
        ((int32_t)sensor->digH5 * humidity)) +
        16384) >> 15) *
        (((((((humidity * (int32_t)sensor->digH6) >> 10) *
        (((humidity * (int32_t)sensor->digH3) >> 11) +
        32768)) >> 10) +
        2097152) *
        (int32_t)sensor->digH2 +
        8192) >> 14));

    humidity -=
        (((((humidity >> 15) *
        (humidity >> 15)) >> 7) *
        (int32_t)sensor->digH1) >> 4);

    if (humidity < 0){
        humidity = 0;
    }

    if (humidity > 419430400){
        humidity = 419430400;
    }

    return (uint32_t)(humidity >> 12);
}

BME280_Status BME280_Read(BME280_Handle *sensor, BME280_Data *data){
    if ((sensor == NULL) ||
        (sensor->hi2c == NULL) ||
        (data == NULL)){
        return BME280_INVALID_ARGUMENT;
    }

    BME280_Status status = BME280_WriteRegister(
        sensor,
        BME280_REGISTER_CTRL_MEAS,
        BME280_CTRL_MEAS_FORCED_X1
    );

    if (status != BME280_OK){
        return status;
    }

    HAL_Delay(2U);
    status = BME280_WaitForMeasurement(sensor);

    if (status != BME280_OK){
        return status;
    }

    uint8_t rawData[8];

    status = BME280_ReadRegisters(
        sensor,
        BME280_REGISTER_DATA,
        rawData,
        sizeof(rawData)
    );

    if (status != BME280_OK){
        return status;
    }

    int32_t rawPressure =
        ((int32_t)rawData[0] << 12) |
        ((int32_t)rawData[1] << 4) |
        ((int32_t)rawData[2] >> 4);

    int32_t rawTemperature =
        ((int32_t)rawData[3] << 12) |
        ((int32_t)rawData[4] << 4) |
        ((int32_t)rawData[5] >> 4);

    int32_t rawHumidity =
        ((int32_t)rawData[6] << 8) |
        (int32_t)rawData[7];

    if ((rawTemperature == 0x80000) ||
        (rawPressure == 0x80000) ||
        (rawHumidity == 0x8000)){
        return BME280_ERROR;
    }

    int32_t temperature = BME280_CompensateTemperature(
        sensor,
        rawTemperature
    );

    uint32_t pressure = BME280_CompensatePressure(
        sensor,
        rawPressure
    );

    uint32_t humidity = BME280_CompensateHumidity(
        sensor,
        rawHumidity
    );

    if (pressure == 0U){
        return BME280_ERROR;
    }

    data->temperature = (float)temperature / 100.0f;
    data->pressure = (float)pressure / 256.0f;
    data->humidity = (float)humidity / 1024.0f;

    return BME280_OK;
}
