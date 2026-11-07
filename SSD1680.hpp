#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

/* Require that all screens have the same setting.
 * Otherwise we might have refresh issues
 * */
#define SPI_MAX_W_SPEED 20000000
#define SPI_DATA_ORDER	MSBFIRST
#define SPI_DATAMODE	SPI_MODE0

#define BLACK 0
#define WHITE 1

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

			if (!refresh()) {
				Serial.printf("refresh failed!");
			}

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
    		fb[y*(width>>3)+(x>>3)]&=~(1<<x%8)|(bool_value<<x%8);
		}
		void clear(uint8_t bool_value) {
			memset(fb, bool_value*0xff, fbsiz);
		}
		bool refresh(void) {
			command(0x22);
			data(0xB1); /* Mode 1 seems to work.*/
			command(0x20);

			WAIT_FAILURE("LUT setting", 5);

			command(0x22);
			data(0xC7);
			command(0x20);
			WAIT_FAILURE("refresh", 5);
			return true;
		}
		/* TODO: FIX partial refresh*/
		/* Partial refresh -> BROKEN! do not use */
		bool refreshPartial(void) {
			command(0x22);
			data(0xB9); 
			command(0x20);

			WAIT_FAILURE("LUT setting", 5);

			command(0x22);
			data(0xCF);
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
		void chrA(uint16_t x, uint16_t y, uint8_t bool_value) {
		
		}
		void bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
				uint8_t bool_value, uint8_t *bm, size_t siz) {

		}
	private:
		/*TODO: rearrange to eliminate most padding */
		static SSD1680 *active_displays;
		static uint8_t active_displays_count; /* restrict to uint8 number of displays.. because who tf has that many screens bruh*/
		uint8_t *fb;
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
		
		uint8_t flip_byte(uint8_t byte) {
			uint8_t LUT[]={
				0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
				0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
				0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
				0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
				0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
				0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
				0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
				0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
				0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
				0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
				0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
				0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
				0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
				0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
				0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
				0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
			};
			return LUT[byte];
		}
};
