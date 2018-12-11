#pragma once

#include <chrono>

namespace RSL
{
	namespace core
	{
		class SpaceNavigator
		{
		public:
			typedef struct _tSpNavData
			{
				short data[6];
				std::chrono::time_point<std::chrono::steady_clock> t;
			} tSpNavData;

			typedef enum : unsigned char
			{
				NO_BUTTON = 0x00,
				BUTTON_1 = 0x01,
				BUTTON_2 = 0x02,
				BUTTON_1_AND_2 = 0x03
			} SpNavButton;

		private:
			class SpNavImpl;
			SpNavImpl *pImpl;
			bool running;

			bool readButtonsSync(SpNavButton &buttons, unsigned long timeout = -1);

		public:

			typedef void(*dataCallback)(tSpNavData&);
			typedef void(*dataCallback2)(short, short, short, short, short, short);
			typedef void(*buttonCallback)(SpNavButton);

			bool isInitialised();
			inline bool isRunning() { return running; }

			SpaceNavigator(void);
			~SpaceNavigator();

			bool readSync(tSpNavData &data, unsigned long timeout = -1);

			bool run(dataCallback dcb, buttonCallback bcb = nullptr);
			bool run(dataCallback2 dcb, buttonCallback bcb = nullptr);
			void stop();

			bool setLED(bool on);

			bool getLED(void);
		};
	}
}

