#include <SPI.h>

class SSD1680 {
	public:
		/* 4 wire spi */
		SSD1680(int SPI_SCL, int SPI_SDA, int SPI_DC, int SPI_CS) {
			scl=SPI_SCL;
			sda=SPI_SDA;
			cs=SPI_CS;
			dc=SPI_DC;
		}
		/* 3 wire spi */
		SSD1680(int SPI_SCL, int SPI_SDA, int SPI_CS) {
			scl=SPI_SCL;
			sda=SPI_SDA;
			cs=SPI_CS;
		}
		void reset(int reset) {rst=reset;}
		int reset(void) {return rst;}
		void busy(int busy) {bsy=busy;}
		int busy(void) {return bsy;}
	private:
		/* CLK, MOSI, CS, DC reset, busy*/
		int scl, sda, cs, dc, rst, bsy;
};
