hexbright
=========

This is the easiest way to get started with programming your hexbright.

First, download and install arduino and the CP210x driver (from http://community.hexbright.com/Wiki/Intro_to_Programming).  Most linux kernels come with the driver pre-built.

Next, download this folder, open the arduino ide, and click on 'File'->'Preferences' in the menu.  
Set your sketchbook location to the location of this folder (where this README file is found).

Restart arduino.

Now, click on 'Tools'->'Board'->'Hexbright' as your device type.
With your hexbright unplugged, go to 'Tools'->'Serial Port' and look at the options.
Now plug in your hexbright and go to 'Tools'->'Serial Port'.  Select the new option.  On linux, there may be a delay of over a minute before the device appears.
Underneath the 'Sketch' and 'Tools' menu options, there is an up arrow (to open a program).  Click on it, go to 'programs', and select a program.

'temperature_calibration' is one of the simplest programs you could write.

'functional' is a basic example of how a program might have multiple modes.

'down_test' contains an example of using the accelerometer.

libraries/hexbright/hexbright.h has a list of all available methods in the api, and is fairly well commented.

Be aware that this library is a work in progress.  In particular, the accelerometer api may change, and it is not yet optimized.

Enjoy!
