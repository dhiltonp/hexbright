file naming convention:
discharge-VOLTAGE_TRIGGER-BRIGHTNESS_LEVEL.log


    R(eset): occurs every 15 seconds
    N(ON): no brightness cap
    F(OFF): full cap (max brightness level=0)


Between each state change are 30 samples, taken over a period of 1/4 of a second.

    R
    N (still on, max level=500)
    .... (30 samples)
    F (switch off, max level=0)
    .... (30 samples)
    N (switch on, max level=BRIGHTNESS_LEVEL)
    .... (30 samples)
    F (switch off, max level=0)
    .... (30 samples)
    N (switch on, max level=500, stay on until the next R)
    .... (30 samples)
    13.75 seconds pass with max level=500 before logging continues


------

Battery charge level prior to these tests varied.  Several other tests were performed while investigating the correlation between avr voltage and battery life, but the data is not available, and for the time being I'm scrapping this extension.

As I wrote in the irc channel:
    
    (10:45:02 AM) dhiltonp: so, I've run a good number of tests, collecting data about how the voltage appears while at various brightnesses and at varying brightness levels
    (10:47:17 AM) dhiltonp: unfortunately, it's not regular enough to accurately predict battery life remaining or even predict shutdown
    (10:49:15 AM) peataway is now known as peatcoal
    (10:49:51 AM) kamiquasi: not even the latter? (didn't expect the former, for reasons outlined before) - darn.. the early forays into that seemed promising :)
    (10:50:01 AM) dhiltonp: well, not completely
    (10:50:49 AM) dhiltonp: it partially depends on the flashlight's brightness
    (10:52:16 AM) dhiltonp: at 501, most values show good power until it shuts off completely
    (10:54:35 AM) dhiltonp: also, the battery itself varies
    (10:55:13 AM) dhiltonp: on a couple of test runs, the light shut off after hitting 2.6 volts
    (10:55:29 AM) dhiltonp: *shut off within 1.5 minutes, anyway
    (10:55:55 AM) kamiquasi: mhm
    (10:56:01 AM) dhiltonp: on another test, it shut off about 1.5 minutes after hitting 2.7 volts, but never hit 2.6
    (10:57:24 AM) dhiltonp: on another, it dropped to 2.8 volts half-way through the test
    (10:57:31 AM) dhiltonp: but then the voltage recovered and never dropped that low again
    (10:58:09 AM) dhiltonp: that was about half an hour before power finally died
    (10:58:38 AM) kamiquasi: weird behavior
    (10:58:42 AM) dhiltonp: yeah
    (10:58:44 AM) peatcoal is now known as peataway
    (10:59:39 AM) dhiltonp: in some tests there is a very obvious general voltage dropping trend
    (11:00:11 AM) dhiltonp: while on others, the voltage fluctuates up and then down by as many as .5 volts within 1/10th of a second
    (11:04:29 AM) dhiltonp: maybe it has to do with how I was recharging the battery and stuff like that
    (11:06:39 AM) dhiltonp: so there is a possibility that the power could be predicted, assuming that the battery was fully charged
    (11:06:50 AM) dhiltonp: and who knows what other constraints would be necessary
    (11:11:32 AM) dhiltonp: predicting battery life from the voltage would probably require forcing brightness to be a specific brightness, too.  This could be really bad in practice
    (11:12:20 AM) dhiltonp: let's say the user has their brightness set at 100 or so, to save power and still be able to see.  Then the light blasts on for 1/4 of a second at 500, ruining their night vision - and repeating this every 15-30 seconds

Basically, estimating % battery remaining from the avr voltage appears to be a no-go.

However, we are able to catch irregularities in voltage that occur, which are generally associated with low power.  We usually notice the issue when the battery is at about 10% remaining (see the 'AVR VOLTAGE' section of <a href="https://github.com/dhiltonp/hexbright/blob/master/libraries/hexbright/hexbright.cpp#L1189">hexbright.cpp</a> for the implementation).
