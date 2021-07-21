# 1-Wire Driver
1-Wire protocol driver for Plan 9 for reading the value of this [temperature sensor](https://www.adafruit.com/product/381). Implementation is defined in `onewire.c`.

# Sample output

```
pu% cat /dev/onewire
0x19c
0x19c
0x19c
0x19c
----- Placed hand on sensor -----
0x19e
0x19f
0x1a0
0x1a2
0x1a3
0x1a5
0x1a7
0x1a8
0x1b2
0x1b4
0x1b5
0x1b7
cpu%
```

# Display

Sample added in `display.c` for displaying sensor value on [44" Color TFT LCD Display](https://www.adafruit.com/product/2088) in the style of a 7-segment display.

![Demo](https://github.com/andreidorin13/onewire-driver/blob/main/sample.gif "Demo")
