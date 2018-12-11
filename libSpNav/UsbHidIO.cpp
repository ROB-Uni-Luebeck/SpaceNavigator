
// UsbHidIO.cpp: implementation of the CUsbHidIO class.
// AUTHOR: AINECC
// DESCRIPTION: Implementation of the CUsbHidIO class. A simple way to Read/write usb - hid devices 

// Based on
// Project: usbhidioc.cpp
// by Jan Axelson (jan@Lvr.co


#include "stdafx.h"
#include "UsbHidIO.h"

CUsbHidIO::CUsbHidIO(void)
{
	DeviceDetected = FALSE; 
	DetailDataFlagDone = 0;
}

CUsbHidIO::~CUsbHidIO(void)
{
	CloseHandles();
}

void CUsbHidIO::CloseHandles()
	{
//	if (hDevInfo != INVALID_HANDLE_VALUE)
//		{
//		SetupDiDestroyDeviceInfoList(hDevInfo);
//		}
	}

DWORD CUsbHidIO::GetHIDCollectionDevices(USHORT &NumHIDDevices, PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData[RD_MAXHIDDEVICES], PHIDD_ATTRIBUTES aAttributes, HIDP_CAPS aNumValueCaps[RD_MAXHIDDEVICES])
{
	PHIDP_PREPARSED_DATA	tmpPreparsedData = nullptr;
	LONG					Result;
	HANDLE					tmpDeviceHandle = INVALID_HANDLE_VALUE;
	HDEVINFO				hDevInfo;
	ULONG					Length;
	ULONG					Required;
	USHORT					HIDDeviceIndex, oldHIDDeviceIndex;
	SP_DEVICE_INTERFACE_DATA	devInfoData;
	PSP_DEVICE_INTERFACE_DATA	pdevInfoData;

	/* 1st STEP ---------------------------------------
	API function: HidD_GetHidGuid
	Get the GUID for all system HIDs.
	Returns: the GUID in HidGuid.
	*/
	HidD_GetHidGuid(&HidGuid);

	/* 2cd STEP ---------------------------------------
	API function: SetupDiGetClassDevs
	Returns: a handle to a device information set for all installed devices.
	Requires: the GUID returned by GetHidGuid.
	*/
	hDevInfo = SetupDiGetClassDevs
	(&HidGuid,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);


	/* 3rd STEP ---------------------------------------
	API function: SetupDiEnumDeviceInterfaces
	Returns: returns TRUE if the function completed without error.
	On return, MyDeviceInterfaceData contains the handle to a SP_DEVICE_INTERFACE_DATA structure for a detected device.
	Requires:
	The DeviceInfoSet returned in SetupDiGetClassDevs.
	The HidGuid returned in GetHidGuid.
	An index to specify a device.
	*/
	devInfoData.cbSize = sizeof(devInfoData);
	HIDDeviceIndex = 0;
	NumHIDDevices = 0;

	// The First round is just for counting the number of HID devices
	do
	{
		Result = SetupDiEnumDeviceInterfaces
		(hDevInfo,
			0,
			&HidGuid,
			HIDDeviceIndex,
			&devInfoData);
		if (Result == TRUE)
		{
			HIDDeviceIndex = HIDDeviceIndex + 1;
		}

	} //do
	while ((Result == TRUE)); // && (DeviceDetected == FALSE))
	NumHIDDevices = HIDDeviceIndex;

	// The second round is just to create a collection with every HID device
	pdevInfoData = (PSP_DEVICE_INTERFACE_DATA)calloc(NumHIDDevices, sizeof(SP_DEVICE_INTERFACE_DATA));
	for (HIDDeviceIndex = 0; HIDDeviceIndex < NumHIDDevices; HIDDeviceIndex++)
	{

		Result = SetupDiEnumDeviceInterfaces
		(hDevInfo,
			0,
			&HidGuid,
			HIDDeviceIndex,
			&devInfoData);
		if (Result == TRUE)
		{
			pdevInfoData[HIDDeviceIndex] = devInfoData;
		}
		else
		{
			pdevInfoData[HIDDeviceIndex].Flags = SPINT_REMOVED;
		}
	}

	/* 4th STEP ---------------------------------------
			API function: SetupDiGetDeviceInterfaceDetail
			Returns: The "path" of each HID Device that is really working in the system
			Requires:
			A DeviceInfoSet returned by SetupDiGetClassDevs
			The SP_DEVICE_INTERFACE_DATA structure returned by SetupDiEnumDeviceInterfaces.
			The final parameter is an optional pointer to an SP_DEV_INFO_DATA structure.
			This application doesn't retrieve or use the structure.
			If retrieving the structure, set
			MyDeviceInfoData.cbSize = length of MyDeviceInfoData.
			and pass the structure's address.

			Usually Leght of the path is less than 255.
			Before doing nothing we release the memory if it was allocated before.
			The parameter "DetailDataFlagDone" stores the last amount of devices counted before,
			that means, the amount of memory allocated
	*/

	if (DetailDataFlagDone > 0 && DetailDataFlagDone < RD_MAXHIDDEVICES)
	{
		for (oldHIDDeviceIndex = 0; oldHIDDeviceIndex < DetailDataFlagDone; oldHIDDeviceIndex++)
		{
			free(aDetailData[oldHIDDeviceIndex]);
		}
	}
	DetailDataFlagDone = HIDDeviceIndex;


	for (HIDDeviceIndex = 0; HIDDeviceIndex < NumHIDDevices; HIDDeviceIndex++)
	{
		// The First round returns the size of the structure in Length.
		Result = SetupDiGetDeviceInterfaceDetail
		(hDevInfo,
			&pdevInfoData[HIDDeviceIndex],
			NULL,
			0,
			&Length,
			NULL);
		if (MAXDEVICEPATH > Length)
		{

			// The second round returns a pointer to the data in DeviceInfoSet.
			aDetailData[HIDDeviceIndex] = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(Length);
			aDetailData[HIDDeviceIndex]->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			Result = SetupDiGetDeviceInterfaceDetail
			(hDevInfo,
				&pdevInfoData[HIDDeviceIndex],
				aDetailData[HIDDeviceIndex],
				Length,
				&Required,
				NULL);
			if (Result != TRUE)	aDetailData[HIDDeviceIndex] = NULL;

			/* 5th STEP ---------------------------------------
				API function: CreateFile
				Returns: a handle that enables reading and writing to each device detected.
				Requires:
				The DevicePath in the detailData structure
				returned by SetupDiGetDeviceInterfaceDetail.
			*/
			tmpDeviceHandle = CreateFile
			(aDetailData[HIDDeviceIndex]->DevicePath,
				0,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				(LPSECURITY_ATTRIBUTES)NULL,
				OPEN_EXISTING,
				0,
				NULL);

			/* 6th STEP ---------------------------------------
				API function: HidD_GetAttributes
				Requests information from the device.
				Requires: the handle returned by CreateFile.
				Returns: a HIDD_ATTRIBUTES structure containing
				the Vendor ID, Product ID, and Product Version Number.
				Use this information to decide if the detected device is
				the one we're looking for.
			*/
			//Set the Size to the number of bytes in the structure.

			aAttributes[HIDDeviceIndex].Size = sizeof(HIDD_ATTRIBUTES);

			Result = HidD_GetAttributes
			(tmpDeviceHandle,
				&aAttributes[HIDDeviceIndex]);

			/* 7bth STEP ---------------------------------------
				API function: HidD_GetPreparsedData
				Returns: a pointer to a buffer containing the information about the device's capabilities.
				Requires: A handle returned by CreateFile.
				There's no need to access the buffer directly,
				but HidP_GetCaps and other API functions require a pointer to the buffer.
			*/

			if (!HidD_GetPreparsedData(tmpDeviceHandle, &tmpPreparsedData))
			{
				aDetailData[HIDDeviceIndex] = nullptr;
				// return GetLastError();
			}

			/* 8th STEP ---------------------------------------
				API function: HidP_GetCaps
				Learn the device's capabilities.
				For standard devices such as joysticks, you can find out the specific
				capabilities of the device.
				For a custom device, the software will probably know what the device is capable of,
				and the call only verifies the information.
				Requires: the pointer to the buffer returned by HidD_GetPreparsedData.
				Returns: a Capabilities structure containing the information.
			*/

			HidP_GetCaps
			(tmpPreparsedData,
				&aNumValueCaps[HIDDeviceIndex]);

			/* 9th STEP ---------------------------------------
				API function: CloseHandle
				We don't need to keep opened every handle of every Device
			*/

		} // If ( MaxDevicePath > Length)
		else
		{
			// TODO Device path lenght is longer than MaxDevicePath
		}
		if (tmpDeviceHandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(tmpDeviceHandle);
			HidD_FreePreparsedData(tmpPreparsedData);
		}

	} // for ( HIDDeviceIndex ++)
	if (hDevInfo != INVALID_HANDLE_VALUE)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		free(pdevInfoData);
	}
	return GetLastError();

}



DWORD CUsbHidIO::GetHIDValueCaps (PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
								  HIDP_REPORT_TYPE ReportType,
								  PHIDP_VALUE_CAPS ValueCaps, USHORT &ValueCapsLength,
								  HANDLE tmpDeviceHandle)
{

	PHIDP_PREPARSED_DATA	tmpPreparsedData;
	HIDP_CAPS				Capabilities;
	BOOL					ExternalHandle;
	DWORD					isValueOK;


/* 1th STEP ---------------------------------------
	 	API function: CreateFile
		The file is being opened or created for asynchronous I/O, so FILE_FLAG_OVERLAPPED is needed.
		When the operation is complete, the event specified in the OVERLAPPED structure is set to the signaled state.
		Returns: a handle that enables reading and writing to each device detected.
		Requires:
		The DevicePath in the detailData structure
		returned by SetupDiGetDeviceInterfaceDetail.
	*/

	if (tmpDeviceHandle == NULL ||tmpDeviceHandle == INVALID_HANDLE_VALUE )
	{
			ExternalHandle = FALSE;
			// If the external handle it is not provided, we create it

		tmpDeviceHandle=CreateFile
				(aDetailData->DevicePath, 
				GENERIC_READ|GENERIC_WRITE, 
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				(LPSECURITY_ATTRIBUTES)NULL, 
				OPEN_EXISTING, 
				FILE_FLAG_OVERLAPPED, 
				NULL);

		if (tmpDeviceHandle != INVALID_HANDLE_VALUE)
		{
			DeviceDetected = TRUE;
		}
		else
		{
			DeviceDetected = FALSE;
			return GetLastError();
		}
	}
	else
	{
		ExternalHandle = TRUE;
	}


	/* 2bth STEP ---------------------------------------
		API function: HidD_GetPreparsedData
		Returns: a pointer to a buffer containing the information about the device's capabilities.
		Requires: A handle returned by CreateFile.
		There's no need to access the buffer directly,
		but HidP_GetCaps and other API functions require a pointer to the buffer.
	*/
	if(!HidD_GetPreparsedData 
		(tmpDeviceHandle, 
		&tmpPreparsedData)) return GetLastError();

	/* 3th STEP ---------------------------------------
		API function: HidP_GetCaps
		Learn the device's capabilities.
		For standard devices such as joysticks, you can find out the specific
		capabilities of the device.
		For a custom device, the software will probably know what the device is capable of,
		and the call only verifies the information.
		Requires: the pointer to the buffer returned by HidD_GetPreparsedData.
		Returns: a Capabilities structure containing the information.
	*/
	
	HidP_GetCaps 
		(tmpPreparsedData, 
		&Capabilities);

	//Display the capabilities
	ValueCapsLength = Capabilities.NumberInputValueCaps;

	/* 4th STEP ---------------------------------------
		API function: HidP_GetValueCaps
		Requires: the pointer to the buffer returned by HidD_GetPreparsedData.
		Returns a value capability array -valueCaps-  that describes all the HID control values
	*/
	isValueOK = (DWORD)HidP_GetValueCaps (ReportType, ValueCaps, &ValueCapsLength, tmpPreparsedData);

	HidD_FreePreparsedData(tmpPreparsedData);
	if (!ExternalHandle)CloseHandle(tmpDeviceHandle);

	if (isValueOK == HIDP_STATUS_SUCCESS)
	{
		return isValueOK;
	}
	else 
	{
		return GetLastError();
	}
}

DWORD CUsbHidIO::GetHIDButtonCaps (PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
								   HIDP_REPORT_TYPE ReportType,
								   PHIDP_BUTTON_CAPS ButtonCaps, USHORT &ButtonCapsLength,
								   HANDLE tmpDeviceHandle)
{

	PHIDP_PREPARSED_DATA	tmpPreparsedData;
	HIDP_CAPS				Capabilities;
	BOOL					ExternalHandle;
	DWORD					isValueOK;



	/* 1th STEP ---------------------------------------
	 	API function: CreateFile
		The file is being opened or created for asynchronous I/O, so FILE_FLAG_OVERLAPPED is needed.
		When the operation is complete, the event specified in the OVERLAPPED structure is set to the signaled state.
		Returns: a handle that enables reading and writing to each device detected.
		Requires:
		The DevicePath in the detailData structure
		returned by SetupDiGetDeviceInterfaceDetail.
	*/

	if (tmpDeviceHandle == NULL ||tmpDeviceHandle == INVALID_HANDLE_VALUE )
	{
			ExternalHandle = FALSE;
			// If the external handle it is not provided, we create it

		tmpDeviceHandle=CreateFile
				(aDetailData->DevicePath, 
				GENERIC_READ|GENERIC_WRITE, 
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				(LPSECURITY_ATTRIBUTES)NULL, 
				OPEN_EXISTING, 
				FILE_FLAG_OVERLAPPED, 
				NULL);

		if (tmpDeviceHandle != INVALID_HANDLE_VALUE)
		{
			DeviceDetected = TRUE;
		}
		else
		{
			DeviceDetected = FALSE;
			return GetLastError();
		}
	}
	else
	{
		ExternalHandle = TRUE;
	}

	/* 2bth STEP ---------------------------------------
		API function: HidD_GetPreparsedData
		Returns: a pointer to a buffer containing the information about the device's capabilities.
		Requires: A handle returned by CreateFile.
		There's no need to access the buffer directly,
		but HidP_GetCaps and other API functions require a pointer to the buffer.
	*/
	if(!HidD_GetPreparsedData 
		(tmpDeviceHandle, 
		&tmpPreparsedData)) return GetLastError();

	/* 3th STEP ---------------------------------------
		API function: HidP_GetCaps
		Learn the device's capabilities.
		For standard devices such as joysticks, you can find out the specific
		capabilities of the device.
		For a custom device, the software will probably know what the device is capable of,
		and the call only verifies the information.
		Requires: the pointer to the buffer returned by HidD_GetPreparsedData.
		Returns: a Capabilities structure containing the information.
	*/
	
	HidP_GetCaps 
		(tmpPreparsedData, 
		&Capabilities);

	//Display the capabilities
	ButtonCapsLength = Capabilities.NumberInputButtonCaps;

	/* 4th STEP ---------------------------------------
		API function: HidP_GetButtonCaps
		Requires: the pointer to the buffer returned by HidD_GetPreparsedData.
		Returns a button capability array -ButtonCaps-  that describes all the HID control values
	*/
	isValueOK = (DWORD)HidP_GetButtonCaps (ReportType, ButtonCaps, &ButtonCapsLength, tmpPreparsedData);

	HidD_FreePreparsedData(tmpPreparsedData);
	if (!ExternalHandle)CloseHandle(tmpDeviceHandle);

	if (isValueOK == HIDP_STATUS_SUCCESS)
	{
		return isValueOK;
	}
	else 
	{
		return GetLastError();
	}
}


DWORD CUsbHidIO::GetHIDUsagesValues (PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
									 HIDP_REPORT_TYPE ReportType, PHIDP_VALUE_CAPS ValueCaps, USHORT ValueCapsLength,
									 PULONG UsageValue, DWORD WaitForMsc, __timeb64* pAcquiredAt,
									 HANDLE	tmpReadHandle)
{

	PHIDP_PREPARSED_DATA	tmpPreparsedData;
	HIDP_CAPS				Capabilities;
	HANDLE					tmphEventObject;
	char					tmpInputReport[1024];
	DWORD					Result;
	DWORD					isValueOK;
	DWORD					NumberOfBytesRead;
	OVERLAPPED				tmpHIDOverlapped;
	BOOL					ExternalHandle;

	isValueOK = ERROR_SUCCESS;

	// Applications must first allocate and zero-initialize the buffer used to return the array of button usages.
	// It is not not necessary to work with reportID -> tmpInputReport[0]=0;
		memset (tmpInputReport, (char) 0, sizeof(tmpInputReport));

	/* 1th STEP ---------------------------------------
	 	API function: CreateFile
		The file is being opened or created for asynchronous I/O, so FILE_FLAG_OVERLAPPED is needed.
		When the operation is complete, the event specified in the OVERLAPPED structure is set to the signaled state.
		Returns: a handle that enables reading and writing to each device detected.
		Requires:
		The DevicePath in the detailData structure
		returned by SetupDiGetDeviceInterfaceDetail.
	*/
	if (tmpReadHandle == NULL ||tmpReadHandle == INVALID_HANDLE_VALUE )
	{
		ExternalHandle = FALSE;
		// If the external handle it is not provided, we create it

		tmpReadHandle=CreateFile
				(aDetailData->DevicePath, 
				GENERIC_READ, 
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				(LPSECURITY_ATTRIBUTES)NULL, 
				OPEN_EXISTING, 
				FILE_FLAG_OVERLAPPED, 
				NULL);

		if (tmpReadHandle != INVALID_HANDLE_VALUE)
		{
			DeviceDetected = TRUE;
		}
		else
		{
			DeviceDetected = FALSE;
			return GetLastError();
		}
	}
	else
	{
		ExternalHandle = TRUE;
	}

	/* 2bth STEP ---------------------------------------
		API function: HidD_GetPreparsedData
		Returns: a pointer to a buffer containing the information about the device's capabilities.
		Requires: A handle returned by CreateFile.
		There's no need to access the buffer directly,
		but HidP_GetCaps and other API functions require a pointer to the buffer.
	*/
	if(!HidD_GetPreparsedData 
		(tmpReadHandle, 
		&tmpPreparsedData)) return GetLastError();

	/* 3th STEP ---------------------------------------
		API function: HidP_GetCaps
		Learn the device's capabilities.
		For standard devices such as joysticks, you can find out the specific
		capabilities of the device.
		For a custom device, the software will probably know what the device is capable of,
		and the call only verifies the information.
		Requires: the pointer to the buffer returned by HidD_GetPreparsedData.
		Returns: a Capabilities structure containing the information.
	*/
	
	HidP_GetCaps 
		(tmpPreparsedData, 
		&Capabilities);


	/* 4th STEP ---------------------------------------
	API function: CreateEvent
	Requires:
	  Security attributes or Null
	  Manual reset (true). Use ResetEvent to set the event object's state to non-signaled.
	  Initial state (true = signaled) 
	  Event object name (optional)
	Returns: a handle to the event object
	*/

//	if (tmphEventObject == 0)
//	{
		tmphEventObject = CreateEvent 
			(NULL, 
			TRUE, 
			TRUE, 
			"");

	//Set the members of the overlapped structure.
	tmpHIDOverlapped.hEvent = tmphEventObject;
	tmpHIDOverlapped.Offset = 0;
	tmpHIDOverlapped.OffsetHigh = 0;
//	}

	/* 5th STEP ---------------------------------------
		API function: ReadFile/HidD_GetInputReport
		Requires: A handle returned by CreateFile.
		Returns the report containing all the information about the specific device.  
	*/

	if (tmpReadHandle != INVALID_HANDLE_VALUE)
		{
		Result = ReadFile 
		(tmpReadHandle, 
		tmpInputReport, 
		Capabilities.InputReportByteLength, 
		&NumberOfBytesRead,
		(LPOVERLAPPED) &tmpHIDOverlapped); 
		}
 	/* option ---------------------------------------
	Result = HidD_GetInputReport
		(tmpDeviceHandle,
		tmpInputReport,
		Capabilities.InputReportByteLength);
	*/

	/* 6th STEP ---------------------------------------
	API function: WaitForSingleObject

	*/
	Result = WaitForSingleObject 
		(tmphEventObject, 
		WaitForMsc);

	/* 7th STEP ---------------------------------------
	API function: HidP_GetUsageValue
	if the 3rd parameter:"LinkCollection" is zero, the routine searches for the usage in the top-level collection associated with PreparsedData
	*/
	switch (Result)
	{
	case WAIT_OBJECT_0:
		{
		for (int i= 0; i<ValueCapsLength;i++)
		{
		isValueOK = (DWORD) HidP_GetUsageValue(ReportType,ValueCaps[i].UsagePage,0,ValueCaps[i].NotRange.Usage,&UsageValue[i],tmpPreparsedData,tmpInputReport,Capabilities.InputReportByteLength);
		}
		if (isValueOK == HIDP_STATUS_SUCCESS)
			{
			_ftime64_s(pAcquiredAt);
			}
		break;
		}
	case WAIT_TIMEOUT:
		{
		CancelIo(tmpReadHandle);
		isValueOK = ERROR_SIGNAL_PENDING;
		break;
		}
	}

	/* 8th STEP ---------------------------------------
	API call: Reset and clear conditions.
	*/

	ResetEvent(tmphEventObject);
	CloseHandle(tmphEventObject);
	HidD_FreePreparsedData(tmpPreparsedData);

	if (!ExternalHandle)CloseHandle(tmpReadHandle);

	if (isValueOK == HIDP_STATUS_SUCCESS || isValueOK == ERROR_SIGNAL_PENDING)
	{
		return isValueOK;
	}
	else 
	{
		return GetLastError();
	}
}


DWORD CUsbHidIO::GetHIDButtonState (PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
									HIDP_REPORT_TYPE ReportType,
									USAGE lcUsagePage, PUSAGE pUsageList, PULONG pUsageLength,
									DWORD WaitForMsc, __timeb64* pAdquiredAt,
									HANDLE	tmpReadHandle)
{

	PHIDP_PREPARSED_DATA	tmpPreparsedData;
	HIDP_CAPS				Capabilities;
	HANDLE					tmphEventObject;
	char					tmpInputReport[1024];
	DWORD					Result;
	DWORD					isValueOK;
	DWORD					NumberOfBytesRead;
	OVERLAPPED				tmpHIDOverlapped;
	BOOL					ExternalHandle;


	isValueOK = ERROR_SUCCESS;

	// Applications must first allocate and zero-initialize the buffer used to return the array of button usages.
	// It is not not necessary to work with reportID -> tmpInputReport[0]=0;
	memset (tmpInputReport, (char) 0, sizeof(tmpInputReport));

	/* 1th STEP ---------------------------------------
	 	API function: CreateFile
		The file is being opened or created for asynchronous I/O, so FILE_FLAG_OVERLAPPED is needed.
		When the operation is complete, the event specified in the OVERLAPPED structure is set to the signaled state.
		Returns: a handle that enables reading and writing to each device detected.
		Requires:
		The DevicePath in the detailData structure
		returned by SetupDiGetDeviceInterfaceDetail.
	*/	
	if (tmpReadHandle == NULL ||tmpReadHandle == INVALID_HANDLE_VALUE )
	{
		ExternalHandle = FALSE;
		// If the external handle it is not provided, we create it

		tmpReadHandle=CreateFile
				(aDetailData->DevicePath, 
				GENERIC_READ, 
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				(LPSECURITY_ATTRIBUTES)NULL, 
				OPEN_EXISTING, 
				FILE_FLAG_OVERLAPPED, 
				NULL);

		if (tmpReadHandle != INVALID_HANDLE_VALUE)
		{
			DeviceDetected = TRUE;
		}
		else
		{
			DeviceDetected = FALSE;
			return GetLastError();
		}
	}
	else
	{
		ExternalHandle = TRUE;
	}

	/* 2bth STEP ---------------------------------------
		API function: HidD_GetPreparsedData
		Returns: a pointer to a buffer containing the information about the device's capabilities.
		Requires: A handle returned by CreateFile.
		There's no need to access the buffer directly,
		but HidP_GetCaps and other API functions require a pointer to the buffer.
	*/

	if(!HidD_GetPreparsedData 
		(tmpReadHandle, 
		&tmpPreparsedData)) return GetLastError();

	/* 3th STEP ---------------------------------------
		API function: HidP_GetCaps
		Learn the device's capabilities.
		For standard devices such as joysticks, you can find out the specific
		capabilities of the device.
		For a custom device, the software will probably know what the device is capable of,
		and the call only verifies the information.
		Requires: the pointer to the buffer returned by HidD_GetPreparsedData.
		Returns: a Capabilities structure containing the information.
	*/
	
	HidP_GetCaps 
		(tmpPreparsedData, 
		&Capabilities);


	/* 4th STEP ---------------------------------------
	API function: CreateEvent
	Requires:
	  Security attributes or Null
	  Manual reset (true). Use ResetEvent to set the event object's state to non-signaled.
	  Initial state (true = signaled) 
	  Event object name (optional)
	Returns: a handle to the event object
	*/	
	tmphEventObject = CreateEvent (NULL, TRUE, TRUE, "");


	/* 5th STEP ---------------------------------------
		API function: ReadFile/HidD_GetInputReport
		Requires: A handle returned by CreateFile.
		Returns the report containing all the information about the specific device.  
	*/
	//Set the members of the overlapped structure.
	tmpHIDOverlapped.hEvent = tmphEventObject;
	tmpHIDOverlapped.Offset = 0;
	tmpHIDOverlapped.OffsetHigh = 0;

	if (tmpReadHandle != INVALID_HANDLE_VALUE)
		{
		Result = ReadFile 
		(tmpReadHandle, 
		tmpInputReport, 
		Capabilities.InputReportByteLength, 
		&NumberOfBytesRead,
		(LPOVERLAPPED) &tmpHIDOverlapped); 
		}
 	/* option ---------------------------------------
	Result = HidD_GetInputReport
		(tmpDeviceHandle,
		tmpInputReport,
		Capabilities.InputReportByteLength);
	*/

	/* 6th STEP ---------------------------------------
	API function: WaitForSingleObject

	*/
	Result = WaitForSingleObject 
		(tmphEventObject, 
		WaitForMsc);

	/* 7th STEP ---------------------------------------
	API function: HidP_GetUsages
	if the 3rd parameter:"LinkCollection" is zero, the routine searches for the usage in the top-level collection associated with PreparsedData
	To call these routines, applications and drivers must first allocate and zero-initialize the buffer used to return the array of button usages.
	An application or driver calls HidP_MaxUsageListLength to determine the number of button usages in a specified usage page in the report.
	If the application or driver specifies a usage page of zero, the routine returns the number of all the button usages in the report.
	*/
 
	pUsageLength[0] = HidP_MaxUsageListLength(ReportType, lcUsagePage,tmpPreparsedData);

	switch (Result)
	{
	case WAIT_OBJECT_0:
		{
		isValueOK = (DWORD) HidP_GetUsages(ReportType, lcUsagePage,0,pUsageList,pUsageLength,tmpPreparsedData,tmpInputReport,Capabilities.InputReportByteLength);
		if (isValueOK == HIDP_STATUS_SUCCESS)
			{
			_ftime64_s(pAdquiredAt);
			}
		break;
		}
	case WAIT_TIMEOUT:
		{
		CancelIo(tmpReadHandle);
		isValueOK = ERROR_SIGNAL_PENDING;
		break;
		}
	}

	/* 8th STEP ---------------------------------------
	API call: Reset and clear conditions.
	*/

	ResetEvent(tmphEventObject);
	CloseHandle(tmphEventObject);
	HidD_FreePreparsedData(tmpPreparsedData);

	if (!ExternalHandle)CloseHandle(tmpReadHandle);

	if (isValueOK == HIDP_STATUS_SUCCESS)
	{
		return isValueOK;
	}
	else 
	{
		return GetLastError();
	}
}

DWORD CUsbHidIO::SetHIDLEDState(PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
								 HIDP_REPORT_TYPE ReportType,
								 USAGE lcUsagePage, PUSAGE pUsageList, PULONG pUsageLength,
								 HANDLE	tmpWriteHandle)
{

	PHIDP_PREPARSED_DATA	tmpPreparsedData;
	HIDP_CAPS				Capabilities;
	PCHAR					tmpOutputReport; //[1024];
	DWORD					Result;
	DWORD					isValueOK;
	DWORD					NumberOfBytesWritten;
	BOOL					ExternalHandle;


	isValueOK = ERROR_SUCCESS;

	/* 1th STEP ---------------------------------------
	 	API function: CreateFile
		The file is being opened or created for asynchronous I/O, so FILE_FLAG_OVERLAPPED is needed.
		When the operation is complete, the event specified in the OVERLAPPED structure is set to the signaled state.
		Returns: a handle that enables reading and writing to each device detected.
		Requires:
		The DevicePath in the detailData structure
		returned by SetupDiGetDeviceInterfaceDetail.
	*/

	if (tmpWriteHandle == NULL ||tmpWriteHandle == INVALID_HANDLE_VALUE )
	{
		ExternalHandle = FALSE;
		// If the external handle it is not provided, we create it

		tmpWriteHandle=CreateFile
				(aDetailData->DevicePath, 
				GENERIC_WRITE, 
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				(LPSECURITY_ATTRIBUTES)NULL, 
				OPEN_EXISTING, 
				0, 
				NULL);

		if (tmpWriteHandle != INVALID_HANDLE_VALUE)
		{
			DeviceDetected = TRUE;
		}
		else
		{
			DeviceDetected = FALSE;
			return GetLastError();
		}
	}
	else
	{
		ExternalHandle = TRUE;
	}


	/* 2bth STEP ---------------------------------------
		API function: HidD_GetPreparsedData
		Returns: a pointer to a buffer containing the information about the device's capabilities.
		Requires: A handle returned by CreateFile.
		There's no need to access the buffer directly,
		but HidP_GetCaps and other API functions require a pointer to the buffer.
	*/
	if(!HidD_GetPreparsedData 
		(tmpWriteHandle, 
		&tmpPreparsedData)) return GetLastError();

	/* 3th STEP ---------------------------------------
		API function: HidP_GetCaps
		Learn the device's capabilities.
		For standard devices such as joysticks, you can find out the specific
		capabilities of the device.
		For a custom device, the software will probably know what the device is capable of,
		and the call only verifies the information.
		Requires: the pointer to the buffer returned by HidD_GetPreparsedData.
		Returns: a Capabilities structure containing the information.
	*/
	
	HidP_GetCaps 
		(tmpPreparsedData, 
		&Capabilities);


	/* 4th STEP ---------------------------------------
	API function: HidP_SetUsages
	if the 3rd parameter:"LinkCollection" is zero, the routine searches for the usage in the top-level collection associated with PreparsedData
	To call these routines, applications and drivers must first allocate and zero-initialize the buffer used to return the array of button usages.
	An application or driver calls HidP_MaxUsageListLength to determine the number of button usages in a specified usage page in the report.
	If the application or driver specifies a usage page of zero, the routine returns the number of all the button usages in the report.
	*/
 
	// All report buffers that are initially sent need to be zero'd out 
	tmpOutputReport = (PCHAR) malloc (Capabilities.OutputReportByteLength);
		// It is not needed work with reportID
		tmpOutputReport[0]=0;

	memset (tmpOutputReport, (char) 0, Capabilities.OutputReportByteLength);
	isValueOK = (DWORD) HidP_SetUsages(ReportType, lcUsagePage,0,pUsageList,pUsageLength,tmpPreparsedData,tmpOutputReport,Capabilities.OutputReportByteLength);

	switch (isValueOK)
	{
	case HIDP_STATUS_SUCCESS:
		isValueOK = HIDP_STATUS_SUCCESS;
	break;
	case HIDP_STATUS_BUFFER_TOO_SMALL:
	case HIDP_STATUS_INVALID_REPORT_LENGTH:
	case HIDP_STATUS_INVALID_REPORT_TYPE:
	case HIDP_STATUS_INCOMPATIBLE_REPORT_ID:
	case HIDP_STATUS_INVALID_PREPARSED_DATA:
	case HIDP_STATUS_USAGE_NOT_FOUND:
			return isValueOK;
	break;
	default:
			isValueOK = HIDP_STATUS_SUCCESS;
	break;
	}

	/* 5th STEP ---------------------------------------
		API function: WritFile
		Requires: A handle returned by CreateFile.
		Sends the report containing all the information about the specific device.  
	*/

	if (tmpWriteHandle != INVALID_HANDLE_VALUE)
		{
		Result = WriteFile 
		(tmpWriteHandle, 
		tmpOutputReport, 
		Capabilities.OutputReportByteLength, 
		&NumberOfBytesWritten,
		NULL); 
		}

	/* 5th STEP ---------------------------------------
	API call: Reset and clear conditions.
	*/

		free (tmpOutputReport);
		HidD_FreePreparsedData(tmpPreparsedData);
	if (!ExternalHandle)CloseHandle(tmpWriteHandle);

	if (isValueOK == HIDP_STATUS_SUCCESS)
	{
		return isValueOK;
	}
	else 
	{
		return GetLastError();
	}
}

DWORD CUsbHidIO::UnSetHIDLEDState(PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
	HIDP_REPORT_TYPE ReportType,
	USAGE lcUsagePage, PUSAGE pUsageList, PULONG pUsageLength,
	HANDLE	tmpWriteHandle)
{

	PHIDP_PREPARSED_DATA	tmpPreparsedData;
	HIDP_CAPS				Capabilities;
	PCHAR					tmpOutputReport; //[1024];
	DWORD					Result;
	DWORD					isValueOK;
	DWORD					NumberOfBytesWritten;
	BOOL					ExternalHandle;


	isValueOK = ERROR_SUCCESS;

	/* 1th STEP ---------------------------------------
		API function: CreateFile
		The file is being opened or created for asynchronous I/O, so FILE_FLAG_OVERLAPPED is needed.
		When the operation is complete, the event specified in the OVERLAPPED structure is set to the signaled state.
		Returns: a handle that enables reading and writing to each device detected.
		Requires:
		The DevicePath in the detailData structure
		returned by SetupDiGetDeviceInterfaceDetail.
	*/

	if (tmpWriteHandle == NULL || tmpWriteHandle == INVALID_HANDLE_VALUE)
	{
		ExternalHandle = FALSE;
		// If the external handle it is not provided, we create it

		tmpWriteHandle = CreateFile
		(aDetailData->DevicePath,
			GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			(LPSECURITY_ATTRIBUTES)NULL,
			OPEN_EXISTING,
			0,
			NULL);

		if (tmpWriteHandle != INVALID_HANDLE_VALUE)
		{
			DeviceDetected = TRUE;
		}
		else
		{
			DeviceDetected = FALSE;
			return GetLastError();
		}
	}
	else
	{
		ExternalHandle = TRUE;
	}


	/* 2bth STEP ---------------------------------------
		API function: HidD_GetPreparsedData
		Returns: a pointer to a buffer containing the information about the device's capabilities.
		Requires: A handle returned by CreateFile.
		There's no need to access the buffer directly,
		but HidP_GetCaps and other API functions require a pointer to the buffer.
	*/
	if (!HidD_GetPreparsedData
	(tmpWriteHandle,
		&tmpPreparsedData)) return GetLastError();

	/* 3th STEP ---------------------------------------
		API function: HidP_GetCaps
		Learn the device's capabilities.
		For standard devices such as joysticks, you can find out the specific
		capabilities of the device.
		For a custom device, the software will probably know what the device is capable of,
		and the call only verifies the information.
		Requires: the pointer to the buffer returned by HidD_GetPreparsedData.
		Returns: a Capabilities structure containing the information.
	*/

	HidP_GetCaps
	(tmpPreparsedData,
		&Capabilities);


	/* 4th STEP ---------------------------------------
	API function: HidP_SetUsages
	if the 3rd parameter:"LinkCollection" is zero, the routine searches for the usage in the top-level collection associated with PreparsedData
	To call these routines, applications and drivers must first allocate and zero-initialize the buffer used to return the array of button usages.
	An application or driver calls HidP_MaxUsageListLength to determine the number of button usages in a specified usage page in the report.
	If the application or driver specifies a usage page of zero, the routine returns the number of all the button usages in the report.
	*/

	// All report buffers that are initially sent need to be zero'd out 
	tmpOutputReport = (PCHAR)malloc(Capabilities.OutputReportByteLength);
	// It is not needed work with reportID
	tmpOutputReport[0] = 0;

	memset(tmpOutputReport, (char)0, Capabilities.OutputReportByteLength);
	isValueOK = (DWORD)HidP_UnsetUsages(ReportType, lcUsagePage, 0, pUsageList, pUsageLength, tmpPreparsedData, tmpOutputReport, Capabilities.OutputReportByteLength);

	switch (isValueOK)
	{
	case HIDP_STATUS_SUCCESS:
		isValueOK = HIDP_STATUS_SUCCESS;
		break;
	case HIDP_STATUS_BUFFER_TOO_SMALL:
	case HIDP_STATUS_INVALID_REPORT_LENGTH:
	case HIDP_STATUS_INVALID_REPORT_TYPE:
	case HIDP_STATUS_INCOMPATIBLE_REPORT_ID:
	case HIDP_STATUS_INVALID_PREPARSED_DATA:
	case HIDP_STATUS_USAGE_NOT_FOUND:
		return isValueOK;
		break;
	default:
		isValueOK = HIDP_STATUS_SUCCESS;
		break;
	}

	/* 5th STEP ---------------------------------------
		API function: WritFile
		Requires: A handle returned by CreateFile.
		Sends the report containing all the information about the specific device.
	*/

	if (tmpWriteHandle != INVALID_HANDLE_VALUE)
	{
		Result = WriteFile
		(tmpWriteHandle,
			tmpOutputReport,
			Capabilities.OutputReportByteLength,
			&NumberOfBytesWritten,
			NULL);
	}

	/* 5th STEP ---------------------------------------
	API call: Reset and clear conditions.
	*/

	free(tmpOutputReport);
	HidD_FreePreparsedData(tmpPreparsedData);
	if (!ExternalHandle)CloseHandle(tmpWriteHandle);

	if (isValueOK == HIDP_STATUS_SUCCESS)
	{
		return isValueOK;
	}
	else
	{
		return GetLastError();
	}
}

DWORD CUsbHidIO::SendHIDReport (PSP_DEVICE_INTERFACE_DETAIL_DATA aDetailData,
								PCHAR pReport,
								HANDLE tmpWriteHandle)
{

	PHIDP_PREPARSED_DATA	tmpPreparsedData;
	HIDP_CAPS				Capabilities;
	DWORD					Result;
	DWORD					NumberOfBytesWritten;
	BOOL					ExternalHandle;

	/* 1th STEP ---------------------------------------
	 	API function: CreateFile
		The file is being opened or created for asynchronous I/O, so FILE_FLAG_OVERLAPPED is needed.
		When the operation is complete, the event specified in the OVERLAPPED structure is set to the signaled state.
		Returns: a handle that enables reading and writing to each device detected.
		Requires:
		The DevicePath in the detailData structure
		returned by SetupDiGetDeviceInterfaceDetail.
	*/	
	if (tmpWriteHandle == NULL ||tmpWriteHandle == INVALID_HANDLE_VALUE )
	{
		ExternalHandle = FALSE;
		// If the external handle it is not provided, we create it

		tmpWriteHandle=CreateFile
				(aDetailData->DevicePath, 
				GENERIC_WRITE, 
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				(LPSECURITY_ATTRIBUTES)NULL, 
				OPEN_EXISTING, 
				0, 
				NULL);

		if (tmpWriteHandle != INVALID_HANDLE_VALUE)
		{
			DeviceDetected = TRUE;
		}
		else
		{
			DeviceDetected = FALSE;
			return GetLastError();
		}
	}
	else
	{
		ExternalHandle = TRUE;
	}


	/* 2bth STEP ---------------------------------------
		API function: HidD_GetPreparsedData
		Returns: a pointer to a buffer containing the information about the device's capabilities.
		Requires: A handle returned by CreateFile.
		There's no need to access the buffer directly,
		but HidP_GetCaps and other API functions require a pointer to the buffer.
	*/

	if(!HidD_GetPreparsedData 
		(tmpWriteHandle, 
		&tmpPreparsedData)) return GetLastError();

	/* 3th STEP ---------------------------------------
		API function: HidP_GetCaps
		Learn the device's capabilities.
		For standard devices such as joysticks, you can find out the specific
		capabilities of the device.
		For a custom device, the software will probably know what the device is capable of,
		and the call only verifies the information.
		Requires: the pointer to the buffer returned by HidD_GetPreparsedData.
		Returns: a Capabilities structure containing the information.
	*/
	
	HidP_GetCaps 
		(tmpPreparsedData, 
		&Capabilities);


	/* 4th STEP ---------------------------------------
			API function: WriteFile
		Requires: A handle returned by CreateFile.
		Sends the report containing all the information about the specific device.  
	*/

	pReport[0]=0;

	if (tmpWriteHandle != INVALID_HANDLE_VALUE)
		{
		Result = WriteFile 
		(tmpWriteHandle, 
		pReport, 
		Capabilities.OutputReportByteLength, 
		&NumberOfBytesWritten,
		NULL); 
		}

	/* 5th STEP ---------------------------------------
	API call: Reset and clear conditions.
	*/

		HidD_FreePreparsedData(tmpPreparsedData);
	if (!ExternalHandle)CloseHandle(tmpWriteHandle);

		return GetLastError();
}









