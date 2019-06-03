using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using RSL.core;

namespace SpaceNavigatorCLI_sharp
{
    class Program
    {
        static bool running = true;
        static clr_SpaceNavigator spNav;

        static void ButtonCallback(clr_SpaceNavigator.SpaceNavigatorButtons button)
        {
            Console.WriteLine(button);

            if ((button & clr_SpaceNavigator.SpaceNavigatorButtons.BUTTON_2) == clr_SpaceNavigator.SpaceNavigatorButtons.BUTTON_2)
                running = false;

            if ((button & clr_SpaceNavigator.SpaceNavigatorButtons.BUTTON_1) == clr_SpaceNavigator.SpaceNavigatorButtons.BUTTON_1)
                spNav.setLED(true);
            else
                spNav.setLED(false);

        }

        static void DataCallback(short z, short y, short x, short rz, short ry, short rx)
        {
            Console.WriteLine(x + " " + y + " " + z + " " + rx + " " + ry + " " + rz);
        }

        static void Main(string[] args)
        {
            Console.WriteLine("SpaceNavigator demo\n\nPress ctrl+c or Button 2 on SpNav to quit. Button 1 will toggle the LED.\n\n");
            spNav = new clr_SpaceNavigator();

            short[] data = new short[6];

            spNav.run(DataCallback, ButtonCallback);

            while(running)
                System.Threading.Thread.Sleep(250);
        }
    }
}
