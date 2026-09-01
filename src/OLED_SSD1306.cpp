/**
 * @file OLED_SSD1306.cpp
 * @brief Implementation of the SSD1306 OLED display driver.
 *
 * Provides low-level I2C communication with an SSD1306-based OLED panel,
 * along with buffer-building routines for rendering text, network status,
 * and battery status glyphs onto the display.
 */

#include "OLED_SSD1306.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <linux/i2c-dev.h>
#include <span>
#include <string>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

/// Number of bytes used to encode a single character glyph.
constexpr int CHAR_LENGTH = 8;

/// Offset subtracted from a character's ASCII value to index into the glyph table.
constexpr int CHAR_INDEX_OFFSET = 31;

/// I2C bus address of the SSD1306 display controller.
constexpr uint8_t I2C_ADDRESS = 0x3C;

/**
 * @brief Write a raw byte buffer to the SSD1306 over I2C.
 *
 * Opens the configured I2C device, selects the SSD1306 slave address,
 * and writes @p length bytes starting at @p data.
 *
 * @param data   Pointer to the byte buffer to send.
 * @param length Number of bytes to write.
 * @return Number of bytes actually written, or -1 on error
 *         (device not found or write failure).
 */
auto SSD1306::_writeData(const uint8_t *data, size_t length) -> ssize_t
{
    const auto I2C_file =
        open(_I2C_device, O_RDWR | O_CLOEXEC); // NOLINT(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    // Connect to the SSD1306 at address 0x3C
    if (ioctl(I2C_file, I2C_SLAVE, I2C_ADDRESS) < 0) // NOLINT(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    {
        std::cerr << "Error: Could not find SSD1306 at " << I2C_ADDRESS << "\n";
        close(I2C_file);
        return -1;
    }
    const ssize_t bytesWritten = ::write(I2C_file, data, length);
    if (bytesWritten < 0)
    {
        std::cerr << "Error writing to I2C device\n";
        return -1; // Indicate an error occurred
    }
    close(I2C_file);     // Close the I2C file descriptor after writing
    return bytesWritten; // Return the number of bytes written
}

/**
 * @brief Send a single command byte to the SSD1306.
 *
 * Prefixes @p command with the SSD1306 command control byte and
 * transmits both bytes over I2C.
 *
 * @param command Command opcode to send to the display controller.
 * @return true if both bytes were written successfully, false otherwise.
 */
auto BaseSSD1306::OLEDSendCommand(uint8_t command) -> bool
{
    const std::array<uint8_t, 2> buffer = {SSD1306_COMMAND, command};
    return (_writeData(buffer.begin(), buffer.size()) == buffer.size());
}

/**
 * @brief Rebuild the display buffer and push it to the OLED.
 *
 * Calls buildBuffer() to regenerate the pixel data, prepends the
 * "data continue" control byte, and writes the full frame to the
 * display over I2C.
 *
 * @return true if the buffer was written successfully, false otherwise.
 */
auto BaseSSD1306::OLEDupdate() -> bool
{
    if (_ready)
    {
        buildBuffer(); // Build the buffer with the current text
        std::vector<uint8_t> buffer;
        buffer.reserve(_OLEDbuffer.size() + 1);

        // Control Byte 0x40 means "all following bytes are pixel RAM data"
        buffer.push_back(SSD1306_DATA_CONTINUE);
        buffer.insert(buffer.end(), _OLEDbuffer.begin(), _OLEDbuffer.end());
        return (_writeData(buffer.data(), buffer.size()) == static_cast<ssize_t>(buffer.size()));
    }
    return false;
}

/**
 * @brief Initialize the SSD1306 display.
 *
 * Sends the standard SSD1306 startup command sequence: turns the
 * display off, configures horizontal addressing mode, sets the full
 * column and page address ranges, turns the display back on, and
 * performs an initial buffer update.
 *
 * @return true if every initialization command was sent successfully,
 *         false if any command failed.
 */
auto BaseSSD1306::begin() -> bool
{
    if (!_ready)
    {
        bool result = OLEDSendCommand(SSD1306_DISPLAY_OFF);         // Display OFF
        result &= OLEDSendCommand(SSD1306_MEMORY_ADDR_MODE);        // Set Memory Addressing Mode
        result &= OLEDSendCommand(SSD1306_HORIZONTAL_ADDRESS_MODE); // 0x00 = Horizontal Addressing Mode
        result &= OLEDSendCommand(SSD1306_SET_COLUMN_ADDR);         // set column address range
        result &= OLEDSendCommand(SSD1306_SET_LOWER_COLUMN);        // start column = 0
        result &= OLEDSendCommand(SSD1306_SET_END_COLUMN);          // end column = 127
        result &= OLEDSendCommand(SSD1306_SET_PAGE_ADDR);           // set page address range
        result &= OLEDSendCommand(SSD1306_SET_START_PAGE);          // start page = 0
        result &= OLEDSendCommand(SSD1306_SET_END_PAGE);            // end page = 7
        result &= OLEDSendCommand(SSD1306_DISPLAY_ON);              // display on
        _ready = true;
        OLEDupdate(); // Update the display with the cleared buffer
        return result;
    }
    return true; // Already initialized
}

/**
 * @brief Render a text string into the OLED pixel buffer.
 *
 * Writes each character of @p text into _OLEDbuffer starting at
 * @p startIndex, advancing by CHAR_LENGTH bytes per character.
 * Rendering stops early once @p maxLength characters have been
 * written, if specified.
 *
 * @param startIndex Byte offset into _OLEDbuffer where rendering begins.
 * @param text       Text string to render.
 * @param maxLength  Maximum number of characters to render; a
 *                    non-positive value (the default, -1) means
 *                    no limit.
 */
void BaseSSD1306::_writeText2Buffer(int startIndex, const std::string &text, int maxLength /* = -1 */)
{
    int offset = startIndex; // NOLINT(misc-const-correctness)
    for (const auto character : text)
    {
        const std::span<const uint8_t> charData{characters.at(character - CHAR_INDEX_OFFSET)};
        std::copy(charData.begin(), charData.end(), std::next(_OLEDbuffer.begin(), offset));
        offset += CHAR_LENGTH; // Move to the next character position
        if (maxLength > 0 && offset >= maxLength * CHAR_LENGTH)
        {
            break; // Stop if we reach the maximum length
        }
    }
}

/**
 * @brief Compose the full OLED pixel buffer for the current display state.
 *
 * Clears and resizes _OLEDbuffer, then renders in sequence:
 *  - The top status text.
 *  - The network status symbol (with a "no network" overlay applied
 *    when _networkOff is set).
 *  - The battery symbol, filled proportionally to _batteryCharge and
 *    with a "tip" indicator drawn when charge is at or above
 *    _batteryTipThreshold.
 *  - The charging indicator glyph, if _batteryCharging is set.
 *  - The main body text.
 *
 * After this call, _OLEDbuffer holds the complete frame ready to be
 * sent to the display via OLEDupdate().
 */
void BaseSSD1306::buildBuffer()
{
    _OLEDbuffer.clear();
    _OLEDbuffer.resize((_OLED_WIDTH * _OLED_HEIGHT) / CHAR_LENGTH, 0x00); // 128x64 / 8 = 1024 bytes
    _writeText2Buffer(0, _topText, _maxTopTextLength);

    // Draw network symbol
    std::vector<uint8_t> networkSymbolData(std::begin(_networkSymbolData), std::end(_networkSymbolData));
    if (_networkOff)
    {
        std::transform(networkSymbolData.begin(), networkSymbolData.end(),
                       std::begin(_networkSymbolBarData), // Automatically resolves to the array start
                       networkSymbolData.begin(), [](uint8_t a, uint8_t b) { // NOLINT(readability-identifier-length)
                           return static_cast<uint8_t>(a | b);               // Keeps bitwise math unsigned
                       });
    }

    std::copy(networkSymbolData.begin(), networkSymbolData.end(),
              std::next(_OLEDbuffer.begin(), static_cast<long>(_networkSymbolStartPage) * CHAR_LENGTH));

    // Draw battery symbol
    /// Battery charge converted from a percentage (0-100) to a fill-segment count.
    const long batteryCharge =
        std::lround(static_cast<float>(_batteryCharge) /
                    10.0F); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

    std::vector<uint8_t> batterySymbolData(std::begin(_emptyBatterySymbolData), std::end(_emptyBatterySymbolData));

    const uint8_t batteryFillMask = 0xFF;
    for (int i = 0; i < batteryCharge && i < _batteryFillLength; ++i)
    {
        batterySymbolData[i + 1] |= batteryFillMask; // Fill the battery from right to left
    }
    const uint8_t batterTipMask = 0x3C;
    constexpr ssize_t batteryTipStart = 11;
    constexpr ssize_t batteryTipEnd = 13;

    if (_batteryCharge >= _batteryTipThreshold)
    {
        for (ssize_t i = batteryTipStart; i <= batteryTipEnd; i++)
        {
            batterySymbolData[i] |= batterTipMask;
        }
    }

    std::copy(batterySymbolData.begin(), batterySymbolData.end(),
              std::next(_OLEDbuffer.begin(), static_cast<long>(_batterySymbolStartPage) * CHAR_LENGTH));

    if (_batteryCharging)
    {
        std::copy(std::begin(_batteryChargingData), std::end(_batteryChargingData),
                  std::next(_OLEDbuffer.begin(), static_cast<long>(_batteryChargingStartPage) * CHAR_LENGTH));
    }
    _writeText2Buffer(_mainTextStartPage * CHAR_LENGTH, _mainText);
}
