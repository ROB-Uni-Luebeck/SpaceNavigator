#include <iostream>
#include "../libSpNav/libSpNav.h"

#include <thread>
#include <mutex>
#include <iomanip>

std::mutex m;
RSL::core::SpaceNavigator spNav;
bool running = true;

void processData(RSL::core::SpaceNavigator::tSpNavData &data)
{
	m.lock();
	std::cout \
		<< std::setw(4) << data.data[0] << " " \
		<< std::setw(4) << data.data[1] << " " \
		<< std::setw(4) << data.data[2] << " " \
		<< std::setw(4) << data.data[3] << " " \
		<< std::setw(4) << data.data[4] << " " \
		<< std::setw(4) << data.data[5] << "\n";
	m.unlock();
}

void buttonCallback(RSL::core::SpaceNavigator::SpNavButton buttons)
{
	m.lock();

	std::cout << "B1:" << ((buttons & RSL::core::SpaceNavigator::BUTTON_1) ? 1 : 0) << " B2:" << ((buttons & RSL::core::SpaceNavigator::BUTTON_2) ? 1 : 0) << "\n";
	
	if (buttons & RSL::core::SpaceNavigator::BUTTON_1)
		spNav.setLED(true);
	else
		spNav.setLED(false);

	if (buttons & RSL::core::SpaceNavigator::BUTTON_2)
		running = false;
	
	m.unlock();
}

int main()
{
	if (spNav.run(processData, buttonCallback))
	{
		std::cout << "SpaceNavigator thread running.\n";

		while (running)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
		}

		std::cout << "SpaceNavigator shut down.\n";
	}
	else
	{
		std::cout << "Failed to initialise SpaceNavigator.\n";
	}
}

