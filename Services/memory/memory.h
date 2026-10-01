#ifndef MEMORY
#define MEMORY

#include "w25q128.h"

void MemoryInit();
void MemoryRead(Sensor_Data *data);
void MemoryWrite(Sensor_Data *data);

#endif
