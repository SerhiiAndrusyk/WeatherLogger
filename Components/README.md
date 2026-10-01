# WeatherLogger STM32U073 port

Components and the display service were ported from H5 to STM32U0 HAL.
The target project retains its generated clock, GPIO, I2C1, SPI1 and RTC setup.
I2C component addresses use HAL format (7-bit address shifted left by one).
TIM2 uses PSC=1599 and ARR=499 at 16 MHz, requesting a display step every 50 ms.
Full-screen blocking I2C transfers may take longer than this interval; pending
ticks are coalesced into a single update. No hardware validation was performed.

Components and Services are source folders for Debug and Release. Refresh and
Clean/Build in CubeIDE to regenerate makefiles after importing these changes.
BME280, EEPROM and W25Q128 APIs are available; the application currently runs
the H5 display menu/scroll demo, not a complete data-logging application.