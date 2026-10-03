#include "logger.h"
#include "main.h"
#include <stdint.h>

extern RTC_HandleTypeDef hrtc;
static WeatherAccumulator accumRecord = {0};
static WeatherAccumulator accumDay = {0};
static WeatherAccumulator accumMonth = {0};
uint32_t countDayRecords = 0;
uint32_t countMonthRecords = 0;
uint32_t countYearRecords = 0;
uint8_t previousHour = 25;
static RTC_DateTypeDef previousDate = {0};


void Logger_Process(){
	RTC_DateTypeDef date;
	RTC_TimeTypeDef time;
	Sensor_Data weather;
	WeatherRecord record;

	HAL_StatusTypeDef status = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	if (status != HAL_OK){
		return;
	}
	status = HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	if (status != HAL_OK){
		return;
	}
	if (time.Hours != previousHour){
		previousHour = time.Hours;
		if (accumRecord.count != 0){
			record.data.temperature = accumRecord.temperatureSum/accumRecord.count;
			record.data.pressure = accumRecord.pressureSum/accumRecord.count;
			record.data.humidity = accumRecord.humiditySum/accumRecord.count;
			record.day = previousDate.Date;
			record.month = previousDate.Month;
			record.year = 2000U + previousDate.Year;
			accumDay.temperatureSum += record.data.temperature;
			accumDay.pressureSum += record.data.pressure;
			accumDay.humiditySum += record.data.humidity;
			accumDay.count++;

			MemoryWrite(&record, (DAYS_ADDRESS + countDayRecords*(sizeof(WeatherRecord))));
			countDayRecords++;
			accumRecord.temperatureSum = 0;
			accumRecord.pressureSum = 0;
			accumRecord.humiditySum = 0;
			accumRecord.count = 0;
		}
	}
	if (date.Date != previousDate.Date){
		if (accumDay.count != 0){
			record.data.temperature = accumDay.temperatureSum/accumDay.count;
			record.data.pressure = accumDay.pressureSum/accumDay.count;
			record.data.humidity = accumDay.humiditySum/accumDay.count;
			record.day = previousDate.Date;
			record.month = previousDate.Month;
			record.year = 2000U + previousDate.Year;
			accumMonth.temperatureSum += record.data.temperature;
			accumMonth.pressureSum += record.data.pressure;
			accumMonth.humiditySum += record.data.humidity;
			accumMonth.count++;
			MemoryWrite(&record, (MONTHS_ADDRESS + countMonthRecords*(sizeof(WeatherRecord))));
			countMonthRecords++;
			accumDay.temperatureSum = 0;
			accumDay.pressureSum = 0;
			accumDay.humiditySum = 0;
			accumDay.count = 0;
		}
	}
	if (date.Month != previousDate.Month){
		if (accumMonth.count != 0){
			record.data.temperature = accumMonth.temperatureSum/accumMonth.count;
			record.data.pressure = accumMonth.pressureSum/accumMonth.count;
			record.data.humidity = accumMonth.humiditySum/accumMonth.count;
			record.day = previousDate.Date;
			record.month = previousDate.Month;
			record.year = 2000U + previousDate.Year;
			MemoryWrite(&record, (YEARS_ADDRESS + countYearRecords*(sizeof(WeatherRecord))));
			countYearRecords++;
			accumMonth.temperatureSum = 0;
			accumMonth.pressureSum = 0;
			accumMonth.humiditySum = 0;
			accumMonth.count = 0;
		}
	}
	SensorRead(&weather);
	accumRecord.temperatureSum += weather.temperature;
	accumRecord.pressureSum += weather.pressure;
	accumRecord.humiditySum += weather.humidity;
	accumRecord.count++;
	previousDate = date;
}
