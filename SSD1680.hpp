#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

/* Require that all screens have the same setting.
 * Otherwise we might have refresh issues
 * */
#define SPI_MAX_W_SPEED 20000000
#define SPI_DATA_ORDER	MSBFIRST
#define SPI_DATAMODE	SPI_MODE0

#define WAIT_FAILURE(WAITER, WAITMAX)  \
	do { \
			while (!waitBusy()) { \
				if (rst_counter>5) { \
					Serial.printf("%s wait exceeded %d times. Failing", WAITER, WAITMAX); \
					return false; \
				}  \
				Serial.printf("%s wait exceeded, retrying: tried %d times.", WAITER, rst_counter); \
				rst_counter++; \
			} \
			rst_counter=0; \
	}while(0)

class SSD1680 {
	public:
		uint8_t *fb;
		SSD1680(uint8_t scl, uint8_t sda, uint8_t res, uint8_t dc, uint8_t cs, uint8_t busy, uint32_t width, uint32_t height):
			scl(scl), sda(sda), res(res), dc(dc), cs(cs), busy(busy), width(width), height(height) {
				fbsiz=height*(width/8); /* width is div by 8 because monochrome screen, consider bits instead */
				fb=(uint8_t *)malloc(sizeof(uint8_t)*fbsiz);
			}
		bool begin(void) {

			rst_counter=0;
			pinMode(cs, OUTPUT);
			pinMode(dc, OUTPUT);
			pinMode(res, OUTPUT);
			pinMode(busy, INPUT);

			digitalWrite(cs, LOW);

			Serial.begin(115200);

			/* Refer to the manual, page 39, Section 9.1
			 * "General Operation Flow to drive display panel"
			 *  */

			/* 2. Set Initial Configuration */
			SPI.begin(scl, -1, sda, cs); /* No miso line, data should be sent back via SDA? fake spi and all */
			SPI.beginTransaction(SPISettings(SPI_MAX_W_SPEED, SPI_DATA_ORDER, SPI_DATAMODE));
			delay(10);

			hwreset();

			swreset();

			WAIT_FAILURE("swreset", 5);

			/* 3. Send Initialization Code */
			command(0x01); /*HEIGHT gate init */
			data((height-1) & 0xff); /* lower 8 bits*/
			data((height-1) >> 8); /* MSB */
			data(0x00); /* Gate scanning sequence/direction */

			command(0x11); /* X/Y RAM entry direction */
			data(0x03); /* X,Y both increment*/

			setRamBoundary(width, height);

			/* Panel border */
			command(0x3C);
			data(0x05); /* for some reason, 0x05 works */

			/* 4. Load Waveform LUT */
			command(0x18);
			data(0x80); /* Use internal temperature sensor */

			command(0x22);
			data(0xB1); /* Mode 1 seems to work. TODO: Sync this up with the display modes */
			command(0x20);

			WAIT_FAILURE("LUT setting", 5);

			/* Moved voltage booster init
			 * from stage 5 to stage 4 as part of init.
			 * */
			command(0x0C); /* POR values */
			data(0x8B);
			data(0x9C);
			data(0x96);
			data(0x0F);
			return true;
		}
		void sendbuf(void) {
			setRamStartAddr(0, 0);

			command(0x24);
			digitalWrite(dc, HIGH);
			digitalWrite(cs, LOW);
			SPI.writeBytes(fb, (width*height)/8);
			digitalWrite(cs, HIGH);
		}
		void pixel(uint16_t x, uint16_t y, uint8_t bool_value) {
    		fb[y*width+(x>>3)]&=~(1<<x%8)|(val<<x%8);
		}
		bool refresh(void) {
			command(0x22);
			data(0xC7);
			command(0x20);
			WAIT_FAILURE("refresh", 5);
			return true;
		}
		void end(void) {
			command(0x10);
			data(0x00);

			SPI.endTransaction();
			SPI.end();
		}
		/* x must be uint8, but y can be uint16, 9 bit */
		void setRamStartAddr(uint16_t x, uint16_t y) {
			command(0x4E); /* Set x, y start position x.. width*/
			data(x);

			command(0x4F); /* Set x, y start position y.. height*/
			data(y & 0xff);
			data(y >> 8);
		}
	private:
		/*TODO: rearrange to eliminate most padding */
		static SSD1680 *active_displays;
		static uint8_t active_displays_count; /* restrict to uint8 number of displays.. because who tf has that many screens bruh*/
		uint32_t width, height, rst_counter;
		uint8_t scl, sda, res, dc, cs, busy; 
		size_t fbsiz;
		void hwreset(void) {
			/* This shouldn't touch the busy pin..?
			 * The manual says nothing about this touching that pin
			 * just wait a conservative amount..
			 * */
			digitalWrite(res, HIGH);
			delay(100);
			digitalWrite(res, LOW);
			delay(20);
			digitalWrite(res, HIGH);
			delay(5000);
		}
		void swreset(void) {
			command(0x12);
		}
		void command(uint8_t cmd) {
			digitalWrite(dc, LOW);
			digitalWrite(cs, LOW);
			SPI.transfer(cmd);
			digitalWrite(cs, HIGH);
		}
		/* Not memory safe! */
		void data(uint8_t *dat, size_t siz) {
			int i;
			digitalWrite(dc, HIGH);
			digitalWrite(cs, LOW);
			for (i=0;i<siz;i++) SPI.transfer(dat[i]);
			digitalWrite(cs, HIGH);
		}
		void data(uint8_t dat) {
			digitalWrite(dc, HIGH);
			digitalWrite(cs, LOW);
			SPI.transfer(dat);
			digitalWrite(cs, HIGH);
		}
		/* Need 16 bit enforcement otherwise int overflow possible. */
		void setRamBoundary(uint16_t wdth, uint16_t hght) {
			/* Screen boundaries */
			command(0x44); /*x ~ width */
			data(0x00);
			data((wdth/8)-1);
			command(0x45); /*y ~ height */
			data(0x00);
			data(0x00);
			data((hght-1) & 0xff); /* 8 lower bits, 0 indexed */
			data((hght-1) >> 8); /* Most significant bit, 0 indexed */
		}
		bool waitBusy(uint32_t wait=10000) {
			uint32_t start=millis();
			/* BUSY is active LOW */
			while (digitalRead(busy)==HIGH) {
				if (millis()-start>wait) return false;
				delay(100);
			}
			return true;
		}
};
