#include <Wire.h>
#include <Arduino.h>



// key points on the light scale
#define MAX_LIGHT_LEVEL 1000
#define MAX_LOW_LEVEL 385
#define CURRENT_LEVEL -1

#define NOW 1


// led constants
#define RLED 0
#define GLED 1

#define LED_OFF 0
#define LED_WAIT 1
#define LED_ON 2

// charging constants
#define CHARGING 0
#define BATTERY 1  
#define CHARGED 2

class hexbright {
  public: 
    // init hardware.
    // ms_delay is the time update will try to wait between runs.
    // the point of ms_delay is to provide regular update speeds,
    // so if code takes longer to execute from one run to the next,
    // the actual interface doesn't change (button click duration,
    // brightness changes). Set this from 5-30. very low is generally
    // fine (or great), BUT if you do any printing, the actual delay
    // will be greater than the value you set.

    hexbright(int ms_delay);
    
    // Put update in your loop().  update() will wait until at least
    //  ms_delay have passed since the previous execution.
    static void update();

    // turn off the hexbright
    static void shutdown();


    // go from start_level to end_level over updates (updates*ms_delay) = ms
    // level is from 0-1000.  The linear scaling algorithm is a Work in progress.
    static void set_light(int start_level, int end_level, int updates);
    // get light level (before overheat protection adjustment)
    static int get_light_level();
    // get light level (after overheat protection adjustment)
    static int get_safe_light_level();

    // Returns the duration the button has been in updates.  Keeps its value 
    //  immediately after being released, allowing for use as follows:
    // if(button_released() && button_held()>500)
    static int button_held();
    // button has been released
    static boolean button_released();
    
    // led = GLED or RLED,
    // state = LED_ON or LED_OFF.  LED_WAIT cannot be directly set.
    // updates = updates before led goes to LED_WAIT state (where it remains for 100 ms)
    static void set_led_state(byte led, byte state, int updates);
    // led = GLED or RLED 
    // returns LED_OFF, LED_WAIT, or LED_ON
    static byte get_led_state(byte led);
    // returns the opposite color than the one passed in
    static byte flip_color(byte color);


    // get raw thermal sensor reading
    static int get_thermal_sensor();
    static int get_celsius();
    static int get_fahrenheit();


    static byte get_charge_state();

    
    // prints a number through the rear leds
    // 120 = 1 red flashes, 2 green flashes, one long red flash (0), 2 second delay.
    // the largest printable value is +/-999,999,999, as the left-most digit is reserved.
    // negative numbers begin with a leading long flash.
    static void print_number(long number);
    // currently printing a number
    static boolean printing_number();

 private: 
    static void adjust_light();
    static void set_light_level(unsigned long level);
    static void overheat_protection();

    static void update_number();

    // controls actual led hardware set.  
    //  As such, state = HIGH or LOW
    static void _set_led(byte led, byte state);
    static void adjust_leds();

    static void read_thermal_sensor();

    static void read_button();

    static void read_accelerometer();
    static void enable_accelerometer();
    static void disable_accelerometer();
};
