#pragma once
#include <windows.h>
#include "uart_operations.h"

// Pure mapping, also used by hardware-free tests. Does not open or alter a port.
bool configure_serial_dcb(DCB& dcb, const SerialControls& options);
