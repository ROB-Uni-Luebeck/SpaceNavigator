#include "stdafx.h"
#include "libSpNav.h"

#include "UsbHidIO.h"
#include <thread>

using namespace RSL::core;

class SpaceNavigator::SpNavImpl
{
	friend class SpaceNavigator;

private:
	SpNavImpl()
	{
		devPath = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(MAXDEVICEPATH);
	}

	~SpNavImpl()
	{
		if(aUsageValue != nullptr)
			free(aUsageValue);
		if(mValueCaps != nullptr)
			free(mValueCaps);
		if(devPath != nullptr)
			free(devPath);
		if (buttonCaps != nullptr)
			free(buttonCaps);
		if (UsageLength != nullptr)
			free(UsageLength);
		if (UsageList != nullptr)
			free(UsageList);
	}

	CUsbHidIO mUsbHidIO;
	int spNavID = -1;
	bool LEDstate = false;

	PSP_DEVICE_INTERFACE_DETAIL_DATA	arrayDetailData[RD_MAXHIDDEVICES];	// Array of "Path structures" with every HID Device
	HIDD_ATTRIBUTES						arrayAttributes[RD_MAXHIDDEVICES];	// Array of "Attributes structures" with every HID Device
	HIDP_CAPS							arrayValueCaps[RD_MAXHIDDEVICES];	// Array of "Capabilities structures" with every HID Device
	USHORT								StrUsageTableInit = 20000;			// This is the pointer to a table of "usage-strings"

	// store data for Space Navigator access
	PSP_DEVICE_INTERFACE_DETAIL_DATA devPath;
	HIDP_CAPS *caps;
	HIDP_BUTTON_CAPS	*buttonCaps = nullptr;
	HIDP_BUTTON_CAPS	*LEDCaps = nullptr;
	PHIDP_VALUE_CAPS mValueCaps = nullptr;
	USHORT mNumValues, mNumButtons, mButtonCapsLength, mLEDCapsLength, numLeds;
	PULONG aUsageValue = nullptr;
	PUSAGE UsageList = nullptr;
	PULONG UsageLength = nullptr;

	std::thread motionThread, buttonThread;
	SpaceNavigator::dataCallback dcb = nullptr;
	SpaceNavigator::dataCallback2 dcb2 = nullptr;
	SpaceNavigator::buttonCallback bcb = nullptr;
};

bool SpaceNavigator::readButtonsSync(SpNavButton &buttons, unsigned long timeout)
{
	if (!isInitialised())
		return false;

	LONG				Result;
	__timeb64			AcquiredAt;

	Result = pImpl->mUsbHidIO.GetHIDButtonState(pImpl->devPath, HidP_Input, pImpl->buttonCaps->UsagePage, pImpl->UsageList, pImpl->UsageLength, timeout, &AcquiredAt, NULL);
	buttons = SpNavButton::NO_BUTTON;

	for (unsigned int i = 0; i < pImpl->UsageLength[0]; i++)
	{
		buttons = (SpNavButton)(buttons | pImpl->UsageList[i]);
	}

	return Result == HIDP_STATUS_SUCCESS;
}

bool SpaceNavigator::isInitialised()
{
	return pImpl != nullptr && (pImpl->spNavID != -1);
}

SpaceNavigator::SpaceNavigator(void)
{
	pImpl = new SpNavImpl();

	ULONG				Result;
	USHORT NumHIDDevices = 0;

	// getting the list of HID devices
	Result = pImpl->mUsbHidIO.GetHIDCollectionDevices(NumHIDDevices, pImpl->arrayDetailData, pImpl->arrayAttributes, pImpl->arrayValueCaps);

	// check for Space Navigator (VID 046D, PID C626)
	for (int i = 0; i < NumHIDDevices; i++)
	{
		if (pImpl->arrayAttributes[i].VendorID == 0x046D && pImpl->arrayAttributes[i].ProductID == 0xC626)
		{
			// found!
			pImpl->spNavID = i;

			// store device capabilities
			pImpl->caps = &pImpl->arrayValueCaps[pImpl->spNavID];

			// store device path
			pImpl->devPath->cbSize = 0;
			pImpl->devPath->cbSize = pImpl->arrayDetailData[pImpl->spNavID]->cbSize;
			
			int PathLength = pImpl->arrayDetailData[pImpl->spNavID]->DevicePath[0]; // first byte is the length

			memcpy(pImpl->devPath->DevicePath,
				pImpl->arrayDetailData[pImpl->spNavID]->DevicePath, PathLength);

			///// ANALOG VALUES /////

			// allocate memory for the list of capabilities
			pImpl->mValueCaps = (PHIDP_VALUE_CAPS)calloc(pImpl->caps->NumberInputValueCaps, sizeof(HIDP_VALUE_CAPS));
			
			// calling the class to get all the capabilities of the device selected
			Result = pImpl->mUsbHidIO.GetHIDValueCaps(pImpl->devPath, HidP_Input, pImpl->mValueCaps, pImpl->mNumValues, NULL);

			// allocate memory for the list of values	
			pImpl->aUsageValue = (PULONG)calloc(pImpl->mNumValues, sizeof(ULONG));

			///// BUTTONS /////

			// allocate memory for the list of button capabilities
			pImpl->buttonCaps = (PHIDP_BUTTON_CAPS)calloc(pImpl->caps->NumberInputButtonCaps, sizeof(HIDP_BUTTON_CAPS));

			// calling the class to get all the capabilities of the device selected
			Result = pImpl->mUsbHidIO.GetHIDButtonCaps(pImpl->devPath, HidP_Input, pImpl->buttonCaps, pImpl->mButtonCapsLength, NULL);

			if (pImpl->buttonCaps->IsRange)
			{
				pImpl->mNumButtons = pImpl->buttonCaps->Range.UsageMax - pImpl->buttonCaps->Range.UsageMin + 1;
			}
			else
			{
				pImpl->mNumButtons = 1;
			}

			pImpl->UsageLength = (PULONG)calloc(pImpl->caps->NumberInputButtonCaps, sizeof(ULONG));

			// allocate memory for the list of button states	
			pImpl->UsageList = (PUSAGE)calloc(pImpl->mNumButtons, sizeof(USAGE));

			///// LED /////

			pImpl->LEDCaps = (PHIDP_BUTTON_CAPS)calloc(pImpl->caps->NumberOutputButtonCaps, sizeof(HIDP_BUTTON_CAPS));

			Result = pImpl->mUsbHidIO.GetHIDButtonCaps(pImpl->devPath, HidP_Output, pImpl->LEDCaps, pImpl->mLEDCapsLength, NULL);

			if (pImpl->LEDCaps->IsRange)
			{
				pImpl->numLeds = pImpl->LEDCaps->Range.UsageMax - pImpl->LEDCaps->Range.UsageMin + 1;
			}
			else
			{
				pImpl->numLeds = 1;
			}

			// turn off LEDs

			PUSAGE				myLEDUsageList;
			PULONG				myLEDUsageListLength;

			myLEDUsageList = (PUSAGE)calloc(pImpl->numLeds, sizeof(USAGE));
			for (int i = 0; i < pImpl->numLeds; i++)
				myLEDUsageList[i] = pImpl->LEDCaps->Range.UsageMin + i;

			myLEDUsageListLength = (PULONG)calloc(1, sizeof(ULONG));
			myLEDUsageListLength[0] = pImpl->numLeds;

			Result = pImpl->mUsbHidIO.UnSetHIDLEDState(pImpl->devPath, HidP_Output, 0x08, myLEDUsageList, myLEDUsageListLength, NULL);

			free(myLEDUsageList);
			free(myLEDUsageListLength);

		}
	}
}

SpaceNavigator::~SpaceNavigator()
{
	stop();
	if (pImpl != nullptr)
		delete pImpl;
}

bool SpaceNavigator::readSync(tSpNavData & data, unsigned long timeout)
{
	if (pImpl == nullptr || pImpl->spNavID == -1)
		return false;

	LONG				result;
	__timeb64			AcquiredAt;

	// calling the class to get all the values of the device selected
	result = pImpl->mUsbHidIO.GetHIDUsagesValues(pImpl->devPath, HidP_Input, pImpl->mValueCaps, pImpl->mNumValues, pImpl->aUsageValue, timeout, &AcquiredAt, NULL);
	if (result == HIDP_STATUS_SUCCESS || result == ERROR_IO_PENDING)
	{
		// store data
		for (int i = 0; i < pImpl->caps->NumberInputValueCaps; i++)
		{
			data.data[i] = (short)(pImpl->aUsageValue[i]);
			data.t = std::chrono::steady_clock::now();
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool SpaceNavigator::run(dataCallback dcb, buttonCallback bcb)
{
	if(!isInitialised())
		return false;

	running = true;
	if (dcb != nullptr)
	{
		pImpl->dcb = dcb;
		pImpl->motionThread = std::thread([&]
		{
			tSpNavData data;
			while (running)
			{
				// constantly poll the space navigator for analog data, timeout 100 msec
				if (readSync(data, 100))
				{
					pImpl->dcb(data);
				}
			}
		});
	}

	if (bcb != nullptr)
	{
		pImpl->bcb = bcb;
		pImpl->buttonThread = std::thread([&]
		{
			SpaceNavigator::SpNavButton buttons = SpaceNavigator::SpNavButton::NO_BUTTON;
			SpaceNavigator::SpNavButton oldButtons = SpaceNavigator::SpNavButton::NO_BUTTON;
			
			while (running)
			{
				// constantly poll the space navigator for button changes, timeout 100 msec
				if (readButtonsSync(buttons, 100))
				{
					if(buttons != oldButtons)
						pImpl->bcb(buttons);
					oldButtons = buttons;
				}
			}
		});
	}

	return true;
}

bool SpaceNavigator::run(dataCallback2 dcb, buttonCallback bcb)
{
	if (!isInitialised())
		return false;

	running = true;
	if (dcb != nullptr)
	{
		pImpl->dcb2 = dcb;
		pImpl->motionThread = std::thread([&]
		{
			tSpNavData data;
			while (running)
			{
				// constantly poll the space navigator for analog data, timeout 100 msec
				if (readSync(data, 100))
				{
					pImpl->dcb2(data.data[0], data.data[1], data.data[2], data.data[3], data.data[4], data.data[5]);
				}
			}
		});
	}

	if (bcb != nullptr)
	{
		pImpl->bcb = bcb;
		pImpl->buttonThread = std::thread([&]
		{
			SpaceNavigator::SpNavButton buttons = SpaceNavigator::SpNavButton::NO_BUTTON;
			SpaceNavigator::SpNavButton oldButtons = SpaceNavigator::SpNavButton::NO_BUTTON;

			while (running)
			{
				// constantly poll the space navigator for button changes, timeout 100 msec
				if (readButtonsSync(buttons, 100))
				{
					if (buttons != oldButtons)
						pImpl->bcb(buttons);
					oldButtons = buttons;
				}
			}
		});
	}

	return true;
}

void SpaceNavigator::stop()
{
	if (isInitialised())
	{
		running = false;
		if(pImpl->dcb != nullptr || pImpl->dcb2 != nullptr)
			pImpl->motionThread.join();
		if(pImpl->bcb != nullptr)
			pImpl->buttonThread.join();
	}
}

bool SpaceNavigator::setLED(bool on)
{
	bool ret = false;

	if (isInitialised())
	{
		USAGE				myLEDUsagePage = 0x08;
		PUSAGE				myLEDUsageList;
		PULONG				myLEDUsageListLength;

		myLEDUsageList = (PUSAGE)calloc(1, sizeof(USAGE));
		myLEDUsageList[0] = pImpl->LEDCaps->Range.UsageMin;

		myLEDUsageListLength = (PULONG)calloc(1, sizeof(ULONG));
		myLEDUsageListLength[0] = 1;

		if (on && !pImpl->LEDstate)
		{
			if (pImpl->mUsbHidIO.SetHIDLEDState(pImpl->devPath, HidP_Output, myLEDUsagePage, myLEDUsageList, myLEDUsageListLength, NULL) == HIDP_STATUS_SUCCESS)
			{
				pImpl->LEDstate = true;
				ret = true;
			}
		}
		else if(!on && pImpl->LEDstate)
		{
			if (pImpl->mUsbHidIO.UnSetHIDLEDState(pImpl->devPath, HidP_Output, myLEDUsagePage, myLEDUsageList, myLEDUsageListLength, NULL) == HIDP_STATUS_SUCCESS)
			{
				pImpl->LEDstate = false;
				ret = true;
			}
		}

		free(myLEDUsageList);
		free(myLEDUsageListLength);
	}

	return ret;
}

bool SpaceNavigator::getLED(void)
{
	if (isRunning())
		return pImpl->LEDstate;
	else
		return false;
}
