#pragma once

// Library includes

#include <array>
#include <cstdint>
#include <string>
#include <vector>

//  SSD1306 Command Set

// Fundamental Commands
#define SSD1306_HORIZONTAL_ADDRESS_MODE 0x00
#define SSD1306_SET_CONTRAST_CONTROL    0x81
#define SSD1306_DISPLAY_ALL_ON_RESUME   0xA4
#define SSD1306_DISPLAY_ALL_ON          0xA5
#define SSD1306_NORMAL_DISPLAY          0xA6
#define SSD1306_INVERT_DISPLAY          0xA7
#define SSD1306_DISPLAY_OFF             0xAE
#define SSD1306_DISPLAY_ON              0xAF
#define SSD1306_NOP                     0xE3

// Scrolling Commands
#define SSD1306_RIGHT_HORIZONTAL_SCROLL              0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL               0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  0x2A
#define SSD1306_DEACTIVATE_SCROLL                    0x2E
#define SSD1306_ACTIVATE_SCROLL                      0x2F
#define SSD1306_SET_VERTICAL_SCROLL_AREA             0xA3

// Addressing Setting Commands
#define SSD1306_SET_LOWER_COLUMN  0x00
#define SSD1306_SET_START_PAGE    0x00
#define SSD1306_SET_END_PAGE      0x07
#define SSD1306_SET_HIGHER_COLUMN 0x10
#define SSD1306_MEMORY_ADDR_MODE  0x20
#define SSD1306_SET_COLUMN_ADDR   0x21
#define SSD1306_SET_PAGE_ADDR     0x22
#define SSD1306_SET_END_COLUMN    0x7F

// Hardware Configuration Commands
#define SSD1306_SET_START_LINE      0x40
#define SSD1306_SET_SEGMENT_REMAP   0xA0
#define SSD1306_SET_MULTIPLEX_RATIO 0xA8
#define SSD1306_COM_SCAN_DIR_INC    0xC0
#define SSD1306_COM_SCAN_DIR_DEC    0xC8
#define SSD1306_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_SET_COM_PINS        0xDA
#define SSD1306_CHARGE_PUMP         0x8D

// Timing & Driving Scheme Setting Commands
#define SSD1306_SET_DISPLAY_CLOCK_DIV_RATIO 0xD5
#define SSD1306_SET_PRECHARGE_PERIOD        0xD9
#define SSD1306_SET_VCOM_DESELECT           0xDB

// I2C related
#define SSD1306_COMMAND       0x00
#define SSD1306_DATA          0xC0
#define SSD1306_DATA_CONTINUE 0x40
#define SSD1306_ADDR          0x3C /**< I2C address alt 0x3D */

// Pixel color
#define BLACK   0
#define WHITE   1
#define INVERSE 2

// Delays
#define SSD1306_INITDELAY 100 /**< Initialisation delay in mS */

/*!
        @brief class to control OLED and define buffer
*/
class BaseSSD1306
{
  public:
    BaseSSD1306() = default;
    ~BaseSSD1306() {};

    auto OLEDupdate() -> bool;
    void OLEDclearBuffer(void);
    void OLEDFillScreen(uint8_t pixel, uint8_t mircodelay);
    void OLEDFillPage(uint8_t page_num, uint8_t pixels, uint8_t delay);
    // OLED_Return_Codes_e  OLEDBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t* data, bool invert);

    auto begin() -> bool;
    bool OLEDSetBufferPtr(uint8_t width, uint8_t height, uint8_t *pBuffer, uint16_t sizeOfBuffer);
    void OLEDinit(void);
    void OLEDPowerDown(void);

    void OLEDEnable(uint8_t on);
    void OLEDContrast(uint8_t OLEDcontrast);
    void OLEDInvert(bool on);
    void setTopText(const std::string &text) { _topText = text; };
    void setNetworkSymbolOff(bool off) { _networkOff = off; };
    void setBatteryCharge(int charge) { _batteryCharge = charge; };
    void setMainText(const std::string &text) { _mainText = text; };
    void setBatteryCharging(bool on) { _batteryCharging = on; };

  protected:
    virtual auto _writeData(const uint8_t *data, size_t length) -> ssize_t = 0;

  private:
    friend class MockedSSD1306;
    friend class SSD1306;
    void _writeText2Buffer(int startIndex, const std::string &text, int maxLength = -1);
    std::string _topText;       /**< Text to display at the top of the OLED screen */
    int _maxTopTextLength = 80; /**< Maximum length of the top text */
    int _networkSymbolStartPage = 11;
    int _batteryChargingStartPage = 13;
    uint8_t _networkSymbolData[12] = {0xC0, 0xC0, 0x00, 0xF0, 0xF0, 0x00,
                                      0xFC, 0xFC, 0x00, 0xFF, 0xFF}; /**< Data for the network symbol */
    uint8_t _networkSymbolBarData[12] = {0x01, 0x03, 0x06, 0x0C, 0x18,
                                         0x30, 0x60, 0xC0, 0x80}; /**< Data for the network symbol bars */
    bool _networkOff = false;                                     /**< Flag to indicate if the network symbol is off */
    int _batterySymbolStartPage = 14;
    uint8_t _emptyBatterySymbolData[16] = {0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
                                           0x81, 0x81, 0x81, 0xE7, 0x24, 0x24, 0x1C};
    static const int _batteryFillLength = 10;
    static const int _batteryTipThreshold = 95;
    int _batteryCharge = 0;
    bool _batteryCharging = false;
    uint8_t _batteryChargingData[8] = {0x88, 0x6C, 0x3E, 0x1B, 0x09};
    int _mainTextStartPage = 32;
    std::string _mainText;
    const char *_I2C_device = "/dev/i2c-3"; /**< I2C device file */
    auto OLEDSendCommand(uint8_t command) -> bool;
    void buildBuffer();
    uint8_t _I2C_address = SSD1306_ADDR; /**< I2C address */
    bool _I2C_DebugFlag = false;         /**< I2C debug flag default false  */
    uint16_t _I2C_ErrorDelay = 100;      /**<I2C delay(in between retry attempts) in event of error in mS*/
    uint8_t _I2C_ErrorRetryNum = 3;      /**< In event of I2C error number of retry attempts*/
    uint8_t _I2C_ErrorFlag = 0x00;       /**< In event of I2C error holds bcm2835 I2C reason code 0x00 = success*/

    uint8_t _OLED_WIDTH = 128;                   /**< Width of OLED Screen in pixels */
    uint8_t _OLED_HEIGHT = 64;                   /**< Height of OLED Screen in pixels */
    uint8_t _OLED_PAGE_NUM = (_OLED_HEIGHT / 8); /**< Number of byte size pages OLED screen is divided into */

    std::vector<uint8_t> _OLEDbuffer;
    bool _ready = false;
};

class SSD1306 : public BaseSSD1306
{
  protected:
    auto _writeData(const uint8_t *data, size_t length) -> ssize_t override;
};
