#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

/* Design: Expect multiple eink screens
 * */
#define SPI_MAX_W_SPEED 20000000
#define SPI_DATA_ORDER	MSBFIRST
#define SPI_DATAMODE	SPI_MODE0

#define BYTE uint8_t

class SSD1680 {
	public:
		SSD1680(uint8_t scl, uint8_t sda, uint8_t res, uint8_t dc, 
				uint8_t cs, uint8_t busy, uint8_t miso, uint32_t width, uint32_t height):
   		scl(scl), sda(sda), res(res), dc(dc), cs(cs), busy(busy), miso(miso), width(width), height(height) {
			pinMode(dc, OUTPUT);
			pinMode(cs, OUTPUT);
			pinMode(res, OUTPUT);
			pinMode(busy, INPUT);
		}
		bool begin(void) {
			delay(10);
			SPI.begin(scl, miso, sda, cs);
			SPI.beginTransaction(SPISettings(SPI_MAX_W_SPEED, SPI_DATA_ORDER, SPI_DATAMODE));

			if (!hwReset()) {
				return false;
			}
			swReset();
			if (!waitBusy()) {
				return false;
			}
			/* Init code, section 3 */
			command(0x01);
			data(0xFF & (height-1)); /* 8 lowest bits */
			data((height-1) >> 8); /* 9th bit */
			data(0x00); /* Default POR value */

			command(0x11); /* Data Entry mode*/
			data(0x03); /* Default POR value */

			/* X is width..? and Y is Height? */
			command(0x44); /* X */
			data(0x00);
			data((width/8)-1);

			command(0x45); /* Y */
			data(0x00);
			data(0x00);
			data(0xFF & (height-1)); /* 8 lowest bits */
			data((height-1) >> 8); /* 9th bit */

			command(0x3C); /* Border waveform whatever.. */
			data(0xC0); /* POR value */

			/* Section 4, load waveform LUT */
			command(0x18);
			data(0x80); /* Internal temperature */

			command(0x22); /* Refresh sequence? */
			data(0xF7);
			command(0x20);
			if (!waitBusy()) {
				return false;
			}
			return true;
		}
		void command(const BYTE command) {
			digitalWrite(dc, LOW);
			digitalWrite(cs, LOW);
			SPI.transfer(command);
			digitalWrite(cs, HIGH);
		}
		void data(const BYTE data) {
			digitalWrite(dc, HIGH);
			digitalWrite(cs, LOW);
			SPI.transfer(data);
			digitalWrite(cs, HIGH);
		}
		void data(const BYTE *data, uint32_t size) {
			digitalWrite(dc, HIGH);
			digitalWrite(cs, LOW);
			SPI.writeBytes(data, size);
			digitalWrite(cs, HIGH);
		}
		bool refresh(void) {
			command(0x22); /* Display Update Control 2, with POR values*/
			data(0xF7);

			command(0x20); /* Master Activation*/
			if (!waitBusy()) return false;
			return true;
		}
		void sleep() {
			command(0x10);
			data(0x01);

			delay(100);
		}
		void end(void) {
			SPI.end();	
		}
	private:
		uint8_t scl, sda, res, dc, cs, busy, miso; 
		uint32_t width, height;
		bool hwReset(void) {
			digitalWrite(res, HIGH);
			delay(20);
			digitalWrite(res, LOW);
			delay(20);
			digitalWrite(res, HIGH);
			delay(20);
			if (!waitBusy()) {
				/*TODO: add some fail logging*/
				return false;
			}
			return true;
		}
		void swReset(void) {
			command(0x12);
		}
		bool waitBusy(uint32_t timeout = 10000) {
			uint32_t start=millis();
			while (digitalRead(busy)==LOW) {
				if (millis() - start > timeout) return false;
				delay(10);
			}
			return true;
		}
};
