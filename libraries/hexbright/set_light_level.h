#ifndef SET_LIGHT_LEVEL_H
#define SET_LIGHT_LEVEL_H

// All set_light_level versions accept values from -1 to 1000 (-1==OFF_LEVEL); 
//  the specific version chosen is called internal to the hexbright library. 
//  Calling directly should work, but you may have some weird bugs.

// A perceptually linear light output function.
//  Sets actual light level, altering value to
//  be perceptually linear, based on steven's 
//  area brightness (cube root)
extern void set_light_level_linear(unsigned long level);

// WIP
// A perceptually linear light output function.
//  Sets actual light level, altering value to
//  be perceptually linear, based on steven's 
//  area brightness (cube root)
// Also tries to avoid glitches as we transition 
//  between low/high output by carefully timing 
//  pwm transitions
extern void set_light_level_smooth(unsigned long level);

// A simple light output function
//  0-500: 0-256 low
//  501-1000: 0-256 high
extern void set_light_level_simple(unsigned long level);



////// actually do selection...
// set_light_level_simple requested?
#ifdef SET_LIGHT_LEVEL_SIMPLE
#define set_light_level set_light_level_simple
#endif

// default to set_light_level_linear
#ifndef set_light_level
#define set_light_level set_light_level_linear
#endif 


#endif // SET_LIGHT_LINEAR_H
