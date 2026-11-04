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
		SSD1680(int scl, int sda, int cs, int dc, int rst, int bsy, int miso) {
			scl=scl; /* SPI */
			sda=sda; /* SPI */
			cs=cs; /* SPI */
			dc=dc; /* OUTPUT */
			rst=rst; /* OUTPUT */
			bsy=bsy; /* INPUT */
			miso=miso; /* SPI (this is unused) */

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
			delay(10); /* force delay 10 ms*/
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
};
