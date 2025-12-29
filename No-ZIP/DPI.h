#pragma once
#include <Windows.h>

int ScaleForDPI(int value, UINT dpi);
UINT GetMonitorDPI(HMONITOR hMonitor);