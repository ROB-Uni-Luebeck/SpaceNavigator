#pragma once

#include <memory>
//#include <afxcoll.h>
// This file is in the Windows DDK available from Microsoft.
#include "RDDefault.h"

#include <wtypes.h>
#include <initguid.h>


extern "C" {

	// This file is in the Windows DDK available from Microsoft.
	#include <hidsdi.h>

	#include <setupapi.h>
	#include <dbt.h>
	#include <sys/timeb.h>
}


class CUsbHidIO  // CWnd Provides the functionality of ON_MESSAGE(WM_DEVICECHANGE, Main_OnDeviceChange)
{
public:
	CUsbHidIO(void);
	~CUsbHidIO(void);


	// HID
	GUID						HidGuid;
	USHORT						DeviceDetected;
	USHORT						DetailDataFlagDone;

public:

	void CloseHandles();
	DWORD GetHIDCollectionDevices(USHORT &NumHIDDevices,
		PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData[RD_MAXHIDDEVICES],
		PHIDD_ATTRIBUTES aAttributes,
		HIDP_CAPS aNumValueCaps[RD_MAXHIDDEVICES]);
	DWORD GetHIDValueCaps(PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
		HIDP_REPORT_TYPE ReportType,
		PHIDP_VALUE_CAPS ValueCaps, USHORT &ValueCapsLength,
		HANDLE DeviceHandle = NULL);
	DWORD GetHIDButtonCaps(PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
		HIDP_REPORT_TYPE ReportType,
		PHIDP_BUTTON_CAPS ButtonCaps, USHORT &ButtonCapsLength,
		HANDLE DeviceHandle = NULL);
	DWORD GetHIDUsagesValues(PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
		HIDP_REPORT_TYPE ReportType,
		PHIDP_VALUE_CAPS ValueCaps, USHORT ValueCapsLength,
		PULONG UsageValue, DWORD WaitForMsc, __timeb64* pAcquiredAt,
		HANDLE ReadHandle = NULL);
	DWORD GetHIDButtonState(PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
		HIDP_REPORT_TYPE ReportType,
		USAGE UsagePage, PUSAGE UsageList, PULONG UsageLength,
		DWORD WaitForMsc, __timeb64* AdquiredAt,
		HANDLE ReadHandle = NULL);
	DWORD SetHIDLEDState(PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
		HIDP_REPORT_TYPE ReportType,
		USAGE UsagePage, PUSAGE UsageList, PULONG UsageLength,
		HANDLE WriteHandle = NULL);
	DWORD UnSetHIDLEDState(PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
		HIDP_REPORT_TYPE ReportType,
		USAGE UsagePage, PUSAGE UsageList, PULONG UsageLength,
		HANDLE WriteHandle = NULL);
	DWORD SendHIDReport(PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
		PCHAR pReport,
		HANDLE tmpWriteHandle = NULL);

};
