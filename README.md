hexbright
=========

This is the easiest way to get started with programming your [hexbright](http://hexbright.com/).

Key Features
------------

*   Automatic overheat protection (adjustable in [hexbright.h OVERHEAT_TEMPERATURE](https://github.com/dhiltonp/hexbright/blob/master/libraries/hexbright/hexbright.h#L90))

*   Automatic low-battery detection (no spontaneous power-offs if too much power is drawn) 

*   All hardware management is done internal to the library; you can focus on firmware features, not implementation.

*   Multiple debug modes to help diagnose problems ([DEBUG in hexbright.h](https://github.com/dhiltonp/hexbright/blob/master/libraries/hexbright/hexbright.h#L71))

Overview
-----------------

*   [libraries/hexbright/hexbright.h](https://github.com/dhiltonp/hexbright/blob/master/libraries/hexbright/hexbright.h) has a list of all available methods in the api, and is fairly well commented.
    <br>(also use it to set [DEBUG modes](https://github.com/dhiltonp/hexbright/blob/master/libraries/hexbright/hexbright.h#L71), [enable printing](https://github.com/dhiltonp/hexbright/blob/master/libraries/hexbright/hexbright.h#L75), or to [save space](https://github.com/dhiltonp/hexbright/blob/master/libraries/hexbright/hexbright.h#L39))

*   ['up_n_down'](https://github.com/dhiltonp/hexbright/tree/master/programs/up_n_down) and ['tactical'](https://github.com/dhiltonp/hexbright/tree/master/programs/tactical) are probably the two most popular programs for daily use.

*   ['temperature_calibration'](https://github.com/dhiltonp/hexbright/tree/master/programs/temperature_calibration) is one of the simplest programs you could write.

*   ['functional'](https://github.com/dhiltonp/hexbright/tree/master/programs/functional) is a basic example of how a program might have multiple modes.

*   ['down_light'](https://github.com/dhiltonp/hexbright/blob/master/hb-examples/down_light/down_light.ino) contains an example of using the accelerometer.


I have translated most of the original sample programs to the library here: https://github.com/dhiltonp/samples

---

   Be aware that this library is a work in progress.  In particular,<br>
*  the accelerometer api will change
*  power consumption has not been optimized ~~is about as good as it is going to get~~ [(see notes)](https://github.com/dhiltonp/hexbright/tree/master/experiments/power_draw#optimizing-power-draw).

Enjoy!

Video Instructions
------------------

[Setting up your development environment](http://www.youtube.com/watch?v=sUbAkz_Lwxk) (development environment setup, minus driver install).

[How to share your code on github](http://www.youtube.com/watch?v=r5VUDEbd08o).

[Writing a program](http://www.youtube.com/watch?v=Q7eRACjCixE).

Text Instructions
-----------------

1.  Download and install [arduino](http://arduino.cc/en/Main/Software) and the CP210x driver (Use a VCP Driver Kit from [here](http://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx)).
    *   For linux: Most kernels come with the driver pre-built (it's been in the kernel since 2005 or so).  **However**, many repositories do not yet have arduino 1.0.1 or higher, which is (currently) required.  The tarballs available at arduino.cc are very easy to work with; I've been able to just extract the files and run arduino.  You will probably have to fiddle with user groups, which vary by distro.

2.  Download this folder/repository using one of the following methods, accessible at the top of the [project page](https://github.com/dhiltonp/hexbright).
    1. Read-Only (no integrated sharing mechanism)
        *   **[ZIP](https://github.com/dhiltonp/hexbright/archive/master.zip)** Extremely easy to get started, but you will need to manually update the library for new features.
        *   **git clone git@github.com:dhiltonp/hexbright.git** Command line, read-only access.
    2. Share your code
        *   **[Fork](https://github.com/dhiltonp/hexbright/fork)** Click fork to generate your own copy of the project (requires a free account on github).
            *   **[Clone in Windows/Mac](github-windows://openRepo/https://github.com/dhiltonp/hexbright)** Great for first-time git users.  Provides a graphical interface in  
                addition to the command line interface with an easy install process.
            *   **git clone git@github.com:USERNAME/hexbright.git** Command line, read-write access.

3.  Open the arduino ide, and click on 'File'->'Preferences' in the menu.

4.  Set your sketchbook location to the location of this folder (where this README file is found).

5.  Restart arduino.

6.  In arduino, click on 'Tools'->'Board'->'Hexbright' as your device type.

7.  With your hexbright unplugged, go to 'Tools'->'Serial Port' and look at the options.

8.  Now plug in your hexbright and go to 'Tools'->'Serial Port'.  Select the new option.  (On linux, there may be a delay of over a minute before the device appears.)

9.  Underneath the 'Sketch' and 'Tools' menu options, there is an up arrow (to open a program).  Click on it, go to 'programs', and select a program.
    *  I recommend tactical as your first program, as it is very straightforward.
    *  All firmwares in the programs directory have a readme describing their operation.  Arduino doesn't have an integrated method of viewing associated files, so you will need to open README.md in the corresponding folder for notes on operation.

10. Under 'Edit', there is a right arrow.  Click it to upload the program.
