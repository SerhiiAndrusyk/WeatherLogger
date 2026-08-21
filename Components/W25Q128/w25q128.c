#include "w25q128.h"

#define W25Q128_COMMAND_JEDEC_ID  0x9FU
#define W25Q128_COMMAND_READ  0x03U
#define W25Q128_COMMAND_WRITE_ENABLE     0x06U
#define W25Q128_COMMAND_READ_STATUS_1    0x05U
#define W25Q128_COMMAND_PAGE_PROGRAM     0x02U
#define W25Q128_COMMAND_SECTOR_ERASE     0x20U
#define W25Q128_STATUS_BUSY              0x01U

#define W25Q128_PAGE_PROGRAM_TIMEOUT_MS  10U
#define W25Q128_SPI_TIMEOUT_MS 500U
#define W25Q128_SECTOR_ERASE_TIMEOUT_MS  1000U

static void W25Q128_Select(W25Q128_Handle *flash){
    HAL_GPIO_WritePin(flash->csPort, flash->csPin, GPIO_PIN_RESET);
}

static void W25Q128_Deselect(W25Q128_Handle *flash){
    HAL_GPIO_WritePin(flash->csPort, flash->csPin, GPIO_PIN_SET);
}
static W25Q128_Status W25Q128_WriteEnable(W25Q128_Handle *flash);
static W25Q128_Status W25Q128_WaitUntilReady(W25Q128_Handle *flash, uint32_t timeout);
static W25Q128_Status W25Q128_ReadStatus1(W25Q128_Handle *flash, uint8_t *registerValue);

W25Q128_Status W25Q128_Init(W25Q128_Handle *flash, SPI_HandleTypeDef *hspi, GPIO_TypeDef *csPort, uint16_t csPin){
    if ((flash == NULL) ||
        (hspi == NULL) ||
        (csPort == NULL)){
        return W25Q128_INVALID_ARGUMENT;
    }

    flash->hspi = hspi;
    flash->csPort = csPort;
    flash->csPin = csPin;

    W25Q128_Deselect(flash);

    uint8_t command = W25Q128_COMMAND_JEDEC_ID;
    uint8_t id[3] = {0xFFU, 0xFFU, 0xFFU};

    W25Q128_Select(flash);

    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(flash->hspi, &command, 1U, W25Q128_SPI_TIMEOUT_MS);

    if (halStatus == HAL_OK){
        halStatus = HAL_SPI_Receive(flash->hspi, id, sizeof(id), W25Q128_SPI_TIMEOUT_MS);
    }

    W25Q128_Deselect(flash);

    if (halStatus == HAL_TIMEOUT){
        return W25Q128_TIMEOUT;
    }

    if (halStatus != HAL_OK){
        return W25Q128_ERROR;
    }

    if ((id[0] != 0xEFU) ||
        (id[1] != 0x40U) ||
        (id[2] != 0x18U)){
        return W25Q128_INVALID_ID;
    }

    return W25Q128_OK;
}

W25Q128_Status W25Q128_Read(W25Q128_Handle *flash, uint32_t address, uint8_t *data, uint32_t size){
	if ((flash == NULL) ||
	    (flash->hspi == NULL) ||
	    (data == NULL) ||
	    (size == 0U) ||
	    (address >= W25Q128_SIZE) ||
	    (size > W25Q128_SIZE - address)){
	    return W25Q128_INVALID_ARGUMENT;
	}
	uint8_t command[4] = {W25Q128_COMMAND_READ, (uint8_t)(address >> 16), (uint8_t)(address >> 8), (uint8_t)address};

	W25Q128_Select(flash);

	HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(flash->hspi, command, sizeof(command), W25Q128_SPI_TIMEOUT_MS);

	while ((halStatus == HAL_OK) && (size > 0U)){
	    uint16_t chunkSize;

	    if (size > UINT16_MAX){
	        chunkSize = UINT16_MAX;
	    } else {
	        chunkSize = (uint16_t)size;
	    }

	    halStatus = HAL_SPI_Receive(flash->hspi, data, chunkSize, W25Q128_SPI_TIMEOUT_MS);

	    if (halStatus == HAL_OK) {
	        data += chunkSize;
	        size -= chunkSize;
	    }
	}

	W25Q128_Deselect(flash);

    if (halStatus == HAL_TIMEOUT){
        return W25Q128_TIMEOUT;
    }

    if (halStatus != HAL_OK){
        return W25Q128_ERROR;
    }

    return W25Q128_OK;
}

static W25Q128_Status W25Q128_ReadStatus1(W25Q128_Handle *flash, uint8_t *registerValue){
    uint8_t command = W25Q128_COMMAND_READ_STATUS_1;

    W25Q128_Select(flash);

    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(flash->hspi, &command, 1U, W25Q128_SPI_TIMEOUT_MS);

    if (halStatus == HAL_OK){
        halStatus = HAL_SPI_Receive(flash->hspi, registerValue, 1U, W25Q128_SPI_TIMEOUT_MS);
    }

    W25Q128_Deselect(flash);

    if (halStatus == HAL_TIMEOUT){
        return W25Q128_TIMEOUT;
    }

    if (halStatus != HAL_OK){
        return W25Q128_ERROR;
    }

    return W25Q128_OK;
}

static W25Q128_Status W25Q128_WaitUntilReady(W25Q128_Handle *flash, uint32_t timeout){
    uint32_t startTick = HAL_GetTick();

    while (HAL_GetTick() - startTick < timeout){
        uint8_t registerValue = 0xFFU;

        W25Q128_Status status = W25Q128_ReadStatus1(flash, &registerValue);

        if (status != W25Q128_OK){
            return status;
        }

        if ((registerValue & W25Q128_STATUS_BUSY) == 0U){
            return W25Q128_OK;
        }
    }

    return W25Q128_TIMEOUT;
}

static W25Q128_Status W25Q128_WriteEnable(W25Q128_Handle *flash){
    uint8_t command = W25Q128_COMMAND_WRITE_ENABLE;

    W25Q128_Select(flash);

    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(flash->hspi, &command, 1U, W25Q128_SPI_TIMEOUT_MS);

    W25Q128_Deselect(flash);

    if (halStatus == HAL_TIMEOUT){
        return W25Q128_TIMEOUT;
    }

    if (halStatus != HAL_OK){
        return W25Q128_ERROR;
    }

    return W25Q128_OK;
}

W25Q128_Status W25Q128_Write(W25Q128_Handle *flash, uint32_t address, const uint8_t *data, uint32_t size){
	if ((flash == NULL) ||
	    (flash->hspi == NULL) ||
	    (data == NULL) ||
	    (size == 0U) ||
	    (address >= W25Q128_SIZE) ||
	    (size > W25Q128_SIZE - address)){
	    return W25Q128_INVALID_ARGUMENT;
	}

	W25Q128_Status status = W25Q128_WaitUntilReady(flash, W25Q128_PAGE_PROGRAM_TIMEOUT_MS);

    if (status != W25Q128_OK){
        return status;
    }

	while (size > 0U){
	    uint16_t pageOffset = (uint16_t)(address % W25Q128_PAGE_SIZE);
	    uint16_t chunkSize = W25Q128_PAGE_SIZE - pageOffset;

	    if (chunkSize > size){
	        chunkSize = (uint16_t)size;
	    }

	    status = W25Q128_WriteEnable(flash);

	    if (status != W25Q128_OK){
	        return status;
	    }

	    uint8_t command[4] = {
	        W25Q128_COMMAND_PAGE_PROGRAM,
	        (uint8_t)(address >> 16),
	        (uint8_t)(address >> 8),
	        (uint8_t)address
	    };

	    W25Q128_Select(flash);

	    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(flash->hspi, command, sizeof(command), W25Q128_SPI_TIMEOUT_MS);

	    if (halStatus == HAL_OK){
	        halStatus = HAL_SPI_Transmit(flash->hspi, (uint8_t *)data, chunkSize, W25Q128_SPI_TIMEOUT_MS);
	    }

	    W25Q128_Deselect(flash);

	    if (halStatus == HAL_TIMEOUT){
	        return W25Q128_TIMEOUT;
	    }

	    if (halStatus != HAL_OK){
	        return W25Q128_ERROR;
	    }

	    status = W25Q128_WaitUntilReady(flash, W25Q128_PAGE_PROGRAM_TIMEOUT_MS);

	    if (status != W25Q128_OK){
	        return status;
	    }

	    address += chunkSize;
	    data += chunkSize;
	    size -= chunkSize;
	}

    return W25Q128_OK;
}

W25Q128_Status W25Q128_EraseSector(W25Q128_Handle *flash, uint32_t address){
	if ((flash == NULL) || (flash->hspi == NULL) || (address >= W25Q128_SIZE)){
		return W25Q128_INVALID_ARGUMENT;
	}
	address -= address % W25Q128_SECTOR_SIZE;
	W25Q128_Status status = W25Q128_WaitUntilReady(flash, W25Q128_SECTOR_ERASE_TIMEOUT_MS);
    if (status != W25Q128_OK){
        return status;
    }

    status = W25Q128_WriteEnable(flash);

    if (status != W25Q128_OK){
        return status;
    }

    uint8_t command[4] = {
        W25Q128_COMMAND_SECTOR_ERASE,
        (uint8_t)(address >> 16),
        (uint8_t)(address >> 8),
        (uint8_t)address
    };

    W25Q128_Select(flash);
    HAL_StatusTypeDef halStatus = HAL_SPI_Transmit(flash->hspi, command, sizeof(command), W25Q128_SPI_TIMEOUT_MS);
    W25Q128_Deselect(flash);

    if (halStatus == HAL_TIMEOUT){
        return W25Q128_TIMEOUT;
    }

    if (halStatus != HAL_OK){
        return W25Q128_ERROR;
    }

    return W25Q128_WaitUntilReady(flash, W25Q128_SECTOR_ERASE_TIMEOUT_MS);
}
