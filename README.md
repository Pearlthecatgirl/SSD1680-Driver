To get started:
Edit the header file to change the SPI settings to be appropriate for your setup:
```c
#define SPI_MAX_W_SPEED 20000000
#define SPI_DATA_ORDER	MSBFIRST
#define SPI_DATAMODE	SPI_MODE0
```

When running simply create a new instance of SSD1680 class, call begin() method.
Remember to call end() method when finishing.

This project is being abandoned for another driver.
