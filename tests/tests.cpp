#include "OLED_SSD1306.hpp"
#include <algorithm>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::Invoke;

class MockedSSD1306 : public BaseSSD1306
{
  public:
    MOCK_METHOD(ssize_t, _writeData, (const uint8_t *data, size_t length), (override));
    void setReady() { _ready = true; };
};

// Verifies that begin() initializes the SSD1306 display with the correct low-level command
// sequence. Captures all _writeData calls and asserts exactly 11 writes occur, checking each
// 2-byte command in order: display off, memory addressing mode, horizontal addressing mode,
// set column address, set lower/end column, set page address, set start/end page, and display
// on — followed by a final 1025-byte write (a SSD1306_DATA_CONTINUE header plus a full blank
// framebuffer) that clears the display.
TEST(OLEDSSD1306Tests, SimpleBeginTest)
{
    MockedSSD1306 displayController;
    const unsigned int expected_num_calls = 11;

    std::vector<std::vector<uint8_t>> all_captured_buffers;
    size_t captured_length = 0;

    EXPECT_CALL(displayController, _writeData(_, _))
        .WillRepeatedly(Invoke(
            [&all_captured_buffers](const uint8_t *data, size_t length) -> ssize_t
            {
                all_captured_buffers.emplace_back(data, data + length);
                return length;
            }));

    ASSERT_TRUE(displayController.begin());

    // Check the calls

    ASSERT_EQ(expected_num_calls, all_captured_buffers.size());

    ASSERT_EQ(2, all_captured_buffers.at(0).size());
    ASSERT_EQ(all_captured_buffers.at(0).at(0), SSD1306_COMMAND);
    ASSERT_EQ(all_captured_buffers.at(0).at(1), SSD1306_DISPLAY_OFF);

    ASSERT_EQ(2, all_captured_buffers.at(1).size());
    ASSERT_EQ(all_captured_buffers.at(1).at(0), SSD1306_COMMAND);
    ASSERT_EQ(all_captured_buffers.at(1).at(1), SSD1306_MEMORY_ADDR_MODE);

    ASSERT_EQ(2, all_captured_buffers.at(2).size());
    ASSERT_EQ(all_captured_buffers.at(2).at(0), SSD1306_COMMAND);
    ASSERT_EQ(all_captured_buffers.at(2).at(1), SSD1306_HORIZONTAL_ADDRESS_MODE);

    ASSERT_EQ(2, all_captured_buffers.at(3).size());
    ASSERT_EQ(all_captured_buffers.at(3).at(0), SSD1306_COMMAND);
    ASSERT_EQ(all_captured_buffers.at(3).at(1), SSD1306_SET_COLUMN_ADDR);

    ASSERT_EQ(2, all_captured_buffers.at(4).size());
    ASSERT_EQ(all_captured_buffers.at(4).at(0), SSD1306_COMMAND);
    ASSERT_EQ(all_captured_buffers.at(4).at(1), SSD1306_SET_LOWER_COLUMN);

    ASSERT_EQ(2, all_captured_buffers.at(5).size());
    ASSERT_EQ(all_captured_buffers.at(5).at(0), SSD1306_COMMAND);
    ASSERT_EQ(all_captured_buffers.at(5).at(1), SSD1306_SET_END_COLUMN);

    ASSERT_EQ(2, all_captured_buffers.at(6).size());
    ASSERT_EQ(all_captured_buffers.at(6).at(0), SSD1306_COMMAND);
    ASSERT_EQ(all_captured_buffers.at(6).at(1), SSD1306_SET_PAGE_ADDR);

    ASSERT_EQ(2, all_captured_buffers.at(7).size());
    ASSERT_EQ(all_captured_buffers.at(7).at(0), SSD1306_COMMAND);
    ASSERT_EQ(all_captured_buffers.at(7).at(1), SSD1306_SET_START_PAGE);

    ASSERT_EQ(2, all_captured_buffers.at(8).size());
    ASSERT_EQ(all_captured_buffers.at(8).at(0), SSD1306_COMMAND);
    ASSERT_EQ(all_captured_buffers.at(8).at(1), SSD1306_SET_END_PAGE);

    ASSERT_EQ(2, all_captured_buffers.at(9).size());
    ASSERT_EQ(all_captured_buffers.at(9).at(0), SSD1306_COMMAND);
    ASSERT_EQ(all_captured_buffers.at(9).at(1), SSD1306_DISPLAY_ON);

    ASSERT_EQ(1025, all_captured_buffers.at(10).size());
    ASSERT_EQ(all_captured_buffers.at(10).at(0), SSD1306_DATA_CONTINUE);
}

// Confirms that OLEDupdate() returns false and does nothing if called before the display has
// been marked ready (i.e., before begin()/setReady() has completed initialization). Guards
// against writing to the display prematurely.
TEST(OLEDSSD1306Tests, UpdateNotReady)
{
    MockedSSD1306 displayController;
    ASSERT_FALSE(displayController.OLEDupdate());
}

// Checks that setTopText() correctly renders a string into the display buffer. After setting
// the top text to "This is a top text" and marking the display ready, it asserts OLEDupdate()
// issues a single 1025-byte write beginning with SSD1306_DATA_CONTINUE, then validates that the
// first 10 characters are each encoded as their correct 8-byte glyph bitmap (looked up from the
// characters font table) at the expected byte offsets in the buffer.
TEST(OLEDSSD1306Tests, TopTextTest)
{
    MockedSSD1306 displayController;
    const std::string topText = "This is a top text";
    displayController.setTopText(topText);
    std::vector<uint8_t> captured_bytes;
    ssize_t captured_length = 0;

    EXPECT_CALL(displayController, _writeData(_, _))
        .WillRepeatedly(Invoke(
            [&captured_bytes, &captured_length](const uint8_t *data, size_t length) -> ssize_t
            {
                captured_bytes.assign(data, data + length);
                captured_length = length;
                return length;
            }));
    displayController.setReady();
    displayController.OLEDupdate();

    ASSERT_EQ(1025, captured_length);
    ASSERT_EQ(SSD1306_DATA_CONTINUE, captured_bytes.at(0));

    for (int i = 0; i < 10; i++)
    {
        std::vector<uint8_t> captured_letter(captured_bytes.begin() + 1 + (i * 8),
                                             captured_bytes.begin() + 9 + (i * 8));
        std::array<uint8_t, 8> actual_letter = characters.at(static_cast<int>(topText[i]) - 31);
        ASSERT_EQ(actual_letter.size(), captured_letter.size());
        ASSERT_TRUE(std::equal(actual_letter.begin(), actual_letter.end(), captured_letter.begin()));
    }
}

// Verifies that with the network status icon enabled (setNetworkSymbolOff(false)) and the
// display ready, calling OLEDupdate() produces a data write of the expected full-buffer length
// (1025 bytes) — a basic smoke test that toggling the network symbol doesn't break the update
// path or buffer size.
TEST(OLEDSSD1306Tests, NetworkEnabledTest)
{
    MockedSSD1306 displayController;
    displayController.setReady();
    displayController.setNetworkSymbolOff(false);
    std::vector<uint8_t> captured_bytes;
    ssize_t captured_length = 0;

    EXPECT_CALL(displayController, _writeData(_, _))
        .WillRepeatedly(Invoke(
            [&captured_bytes, &captured_length](const uint8_t *data, size_t length) -> ssize_t
            {
                captured_bytes.assign(data, data + length);
                captured_length = length;
                return length;
            }));

    displayController.OLEDupdate();
    ASSERT_EQ(1025, captured_length);
}
