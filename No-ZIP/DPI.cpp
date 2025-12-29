#include <shellscalingapi.h>
#include "DPI.h"

int ScaleForDPI(int value, UINT dpi) {
	return MulDiv(value, dpi, 96);
}

UINT GetMonitorDPI(HMONITOR hMonitor) {
	UINT dpiX, dpiY;
	if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) return dpiX;
	return 96;
}