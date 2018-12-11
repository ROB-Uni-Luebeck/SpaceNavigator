#pragma once

using namespace System;
using namespace System::Runtime::InteropServices;

#include "../libSpNav/libSpNav.h"

namespace RSL
{
	namespace core 
	{
		public ref class clr_SpaceNavigator
		{
		private:
			SpaceNavigator *m_spnav;
			SpaceNavigator::tSpNavData *_data;
			
		public:
			enum class SpaceNavigatorButtons : unsigned char
			{
				NO_BUTTON = 0x00,
				BUTTON_1 = 0x01,
				BUTTON_2 = 0x02,
				BUTTON_1_AND_2 = 0x03
			}; 
			
			delegate void DataCallback(short tz, short ty, short tx, short rz, short ry, short rx);
			delegate void ButtonCallback(SpaceNavigatorButtons buttonState);

			clr_SpaceNavigator() { m_spnav = new SpaceNavigator(); _data = new SpaceNavigator::tSpNavData(); }
			~clr_SpaceNavigator() { this->!clr_SpaceNavigator(); }
			!clr_SpaceNavigator() { delete m_spnav; delete _data; }

			bool isInitialised() { return m_spnav->isInitialised(); }
			bool isRunning() { return m_spnav->isRunning(); }

			bool readSync(array<short> ^%data, unsigned long timeout) 
			{ 
				if (m_spnav->readSync(*_data, timeout))
				{
					if (data->Length < 6)
						throw gcnew Exception("Invalid array size!");

					for(int i=0; i<6; i++)
						data[i] = _data->data[i];
					
					return true;
				}
				else
					return false;
			}

			void stop() { m_spnav->stop(); }

			bool setLED(bool on) { return m_spnav->setLED(on); }

			bool getLED(void) { return m_spnav->getLED(); }

			bool run(DataCallback ^dcb, ButtonCallback ^bcb)
			{
				SpaceNavigator::buttonCallback _bcb = (SpaceNavigator::buttonCallback)Marshal::GetFunctionPointerForDelegate(bcb).ToPointer();
				SpaceNavigator::dataCallback2 _dcb = (SpaceNavigator::dataCallback2)Marshal::GetFunctionPointerForDelegate(dcb).ToPointer();
				return m_spnav->run(_dcb, _bcb);
			}
		};
	}
}
