Planned structure in moving to a framework:

3 key directories:

hb_modes - a collection of light modes (like light_spin, light_down, strobe_serial, selector_tap)<br>
hb_utilities - things like print_number, click_counter, advanced
accelerometer interfaces. In general, code that expands on the core
library without any dependencies.<br>
hexbright - core library, plus .h files for overridable
options/behavior (low_battery, overheat, set_light_level)<br>

hexbright functions by itself, but hb_utilities provide convenience functions and in some cases alter the behavior of the core library. 

hb_modes simply provides an easy way to customize and share code.
