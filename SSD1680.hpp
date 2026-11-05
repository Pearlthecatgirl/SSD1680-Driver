/* Depends on base arduino libraries. Made for ESP32-S3 (tested on lilygo t-display-s3)*/
#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

#define SPI_MAX_W_SPEED 20000000
#define SPI_DATA_ORDER	MSBFIRST
#define SPI_DATAMODE	SPI_MODE0

#define BYTE uint8_t
#define SWRESET 0x12

class SSD1680 {
	public:
		SSD1680(uint8_t scl, uint8_t sda, uint8_t cs, uint8_t dc, uint8_t rst, uint8_t bsy, uint8_t miso
				int width, int height) {
			scl=scl; /* SPI */
			sda=sda; /* SPI */
			cs=cs; /* SPI */
			dc=dc; /* OUTPUT */
			rst=rst; /* OUTPUT */
			bsy=bsy; /* INPUT */
			miso=miso; /* SPI (this is unused) */
			width=width;
			height=height;

			pinMode(dc, OUTPUT);
			pinMode(rst, OUTPUT);
			pinMode(bsy, INPUT);
		}
		void begin(void) {
			delay(10); /* force delay 10 ms*/
			SPI.begin(scl, miso, sda, cs);
			SPI.beginTransaction(SPI_MAX_W_SPEED, SPI_DATA_ORDER, SPI_DATAMODE);

			/* Refer to the operation flow section of SSD1680 manual*/
			digitalWrite(rst, HIGH);
			command(0x12); /*SW reset */
			delay(10); /* force delay 10 ms*/

			/* 3. Init Code */
			command(0x01); /* Gate driver output */
			/* Init All lines, aka 295 */
			data((height-1) & 0xFF); /* A[7:0] */
			data((height-1) >> 8); /* 0..A8 */
			data(0x0); /* Use POR value*/

			command(0x11);
			data(0x3); /* A2 -> direction, default X*/
						/* TODO: Orientation of screen ?*/

			/* Ram X Bounds*/
			command(0x44);
			data(0x0); /* Start at 0 */
			data((height>>3)-1); /* group by 8, divide by 8, same as bitshift 3*/

			/* Ram Y Bounds*/
			command(0x45);
			data(0x0); /* Start at 0, 9 bits MSB first */
			data(0x0);
			data((height-1) & 0xFF); /* End at 296 */
			data((height-1) >> 8);

			/* Border Waveform Control*/
			command(0x3C);
			data(0xC0); /* POR values because idk what is going on */

			waitBusy();
		}
		/* Might not be so good to expose command and data to the user?*/
		void command(const BYTE command) {
			digitalWrite(this->SPI_DC(), LOW);
			digitalWrite(this->SPI_CS(), LOW);
			SPI.transfer(command);
			digitalWrite(this->SPI_CS(), HIGH);
		}
		void data(const BYTE data) {
			digitalWrite(this->SPI_DC(), HIGH);
			digitalWrite(this->SPI_CS(), LOW);
			SPI.transfer(data);
			digitalWrite(this->SPI_CS(), HIGH);
		}
		void end(void) {
			SPI.end();	
		}
		/* Just some oop bs*/
		uint8_t SPI_SCL(void) {return scl;}
		void SPI_SCL(uint8_t SCL) {scl=SCL;}
		uint8_t SPI_SDA(void) {return sda;}
		void SPI_SDA(uint8_t SDA) {sda=SDA;}
		uint8_t SPI_CS(void) {return cs;}
		void SPI_CS(uint8_t CS) {cs=CS;}
		uint8_t SPI_DC(void) {return dc;}
		void SPI_DC(uint8_t DC) {dc=DC;}
		uint8_t SPI_RST(void) {return rst;}
		void SPI_RST(uint8_t RST) {rst=RST;}
		uint8_t SPI_BSY(void) {return bsy;}
		void SPI_BSY(uint8_t BSY) {bsy=BSY;}
		uint8_t SPI_MISO(void) {return miso;}
		void SPI_MISO(uint8_t MISO) {miso=MISO;}
	private:
		/* CLK, MOSI, CS, DC reset, busy, MISO*/
		uint8_t scl, sda, cs, dc, rst, bsy, miso;

		void waitBusy(uint32_t timeout = 10000) {
        	uint32_t start = millis();

        	// SSD1680 BUSY is normally active LOW.
        	while (digitalRead(_busy) == LOW) {
        	    if (millis() - start > timeout) {
        	        break;
        	    }
        	delay(1);
        }
    }


};
