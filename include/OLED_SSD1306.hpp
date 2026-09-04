#pragma once

// Library includes

#include <array>
#include <cstdint>
#include <string>
#include <vector>

//  SSD1306 Command Set

// Fundamental Commands

// Tells the controller how GDDRAM addresses increment as you write
// pixel data. 0x00 = Horizontal addressing mode (column pointer auto-increments,
// then wraps to next page). Other options are 0x01 (vertical) and 0x02 (page
// addressing, the older/simpler mode many libraries default to).
#define SSD1306_HORIZONTAL_ADDRESS_MODE {0x20, 0x00}

// Controls left/right orientation. 0xA1 maps column address 127 to SEG0 (i.e., flips it horizontally). 0xA0 would be
// the non-flipped default. This is a "no parameter" command — the direction is baked into which of the two opcodes (A0
// or A1) you send.
#define SSD1306_SET_CONTRAST_CONTROL {0x81, 0xcf}

// Controls left/right orientation. 0xA1 maps column address 127 to SEG0 (i.e., flips it horizontally). 0xA0 would be
// the non-flipped default. This is a "no parameter" command — the direction is baked into which of the two opcodes (A0
// or A1) you send.
#define SSD1306_SEGMENT_REMAP {0xa1}

// Sets normal (non-inverted) display mode — 1 = pixel on, 0 = pixel off. The alternative, 0xA7, is inverse mode. No
// parameter.
#define SSD1306_NORMAL_DISPLAY {0xa6}

// Tells the controller how many display rows (COM lines) are actually active — critical for correct operation. 0x3F =
// 63 decimal, meaning 64 rows (0–63), which matches a 128×64 panel. If your display is 128×32, this should be 0x1F
// instead. This is the one to double check for your specific panel.
#define SSD1306_MULTIPLEX_RATIO {0xa8, 0x3f}

#define SSD1306_DISPLAY_OFF {0xae}
#define SSD1306_DISPLAY_ON  {0xaf}

// Controls top/bottom orientation, similar concept to A1 but for rows. 0xC8 scans from COM[N-1] to COM0 (flipped
// vertically); 0xC0 would be the default direction. No parameter — again baked into the opcode choice.
#define SSD1306_COM_SCAN_DIR_DEC {0xc8}

// Tells the controller the physical COM pin layout of the panel (alternative vs sequential, left/right remap). 0x12 is
// standard for 128×64 panels; 128×32 panels typically need 0x02 instead.
#define SSD1306_SET_COM_PINS {0xda, 0x12}

// Enables the internal DC-DC charge pump that generates the higher voltage needed to actually drive the OLED pixels.
// 0x14 = enable. 0x10 would leave it disabled. Without this correctly set, the panel has no drive voltage and will stay
// completely dark no matter what else is configured correctly — which is why this was the most important one to fix in
// your original sequence.
#define SSD1306_CHARGE_PUMP {0x8d, 0x14}

// One byte packs two values: the low nibble is the clock divide ratio, the high nibble is the oscillator frequency.
// 0x80 is the standard reset/default value most manufacturers recommend.
#define SSD1306_SET_DISPLAY_CLOCK_DIV_RATIO {0xd5, 0x80}

// Controls the pre-charge and discharge phase timing for each pixel, which affects contrast/ghosting. High nibble =
// phase 2 (discharge), low nibble = phase 1 (pre-charge). 0xF1 is the value commonly used for displays with an internal
// (not external) VCC supply.
#define SSD1306_SET_PRECHARGE_PERIOD {0xd9, 0xf1}

// Sets the voltage level used to drive "deselected" rows, which affects contrast/brightness perception. 0x40 is the
// commonly used default.
#define SSD1306_SET_VCOM_DESELECT {0xdb, 0x40}

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

enum class NetworkSymbol
{
    HIDDEN,
    NETWORK_OFF,
    NETWORK_ON,
};

enum class BatterySymbol
{
    HIDDEN,
    NORMAL,
    UNKOWN,
};

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
    void setNetworkSymbolOff(bool off);
    void setBatteryCharge(int charge);
    void setMainText(const std::string &text) { _mainText = text; };
    void setBatteryCharging(bool on) { _batteryCharging = on; };

  protected:
    virtual auto _writeData(const uint8_t *data, size_t length) -> ssize_t = 0;

  private:
    friend class MockedSSD1306;
    friend class SSD1306;
    void _writeText2Buffer(int startIndex, const std::string &text, int maxLength = -1);
    void DrawNetworkSymbol();
    void DrawBatterySymbol();
    std::string _topText;       /**< Text to display at the top of the OLED screen */
    int _maxTopTextLength = 80; /**< Maximum length of the top text */
    int _networkSymbolStartPage = 11;
    int _batteryChargingStartPage = 13;
    uint8_t _networkSymbolData[12] = {0xC0, 0xC0, 0x00, 0xF0, 0xF0, 0x00,
                                      0xFC, 0xFC, 0x00, 0xFF, 0xFF}; /**< Data for the network symbol */
    uint8_t _networkSymbolBarData[12] = {0x01, 0x03, 0x06, 0x0C, 0x18,
                                         0x30, 0x60, 0xC0, 0x80}; /**< Data for the network symbol bars */
    NetworkSymbol _networkSymbol = NetworkSymbol::HIDDEN;         /**< Flag to indicate if the network symbol is off */
    BatterySymbol _batterySymbol = BatterySymbol::HIDDEN;         /**< Flag to indicate if the battery symbol is off */
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
    auto OLEDSendCommand(std::vector<uint8_t> command) -> bool;
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
