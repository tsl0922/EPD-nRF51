#include <NimBLELog.h>
#include "font.h"
#include "lunar.h"
#include "EPD.h"

GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
static uint16_t partialUpdates = 0;

struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
} onscreen;

void EPDInit()
{
    display.init();
    u8g2Fonts.begin(display);
}

void EPDClear()
{
    display.setFullWindow();
    display.writeScreenBuffer();
    display.refresh();
}

static void drawDateHeader(int16_t x, int16_t y, tm_t &tm, struct Lunar_Date &Lunar)
{
    u8g2Fonts.setCursor(x, y);
    u8g2Fonts.setFont(u8g2_font_wqy12b_t_lunar);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    u8g2Fonts.printf("%d年%02d月%02d日 星期%s", tm.tm_year + YEAR0, tm.tm_mon + 1, tm.tm_mday, Lunar_DayString[tm.tm_wday]);

    LUNAR_SolarToLunar(&Lunar, tm.tm_year + YEAR0, tm.tm_mon + 1, tm.tm_mday);
    u8g2Fonts.setFont(u8g2_font_wqy9_t_lunar);
    u8g2Fonts.setCursor(x + 226, y);
    u8g2Fonts.printf("农历: %s%s%s %s%s[%s]年", Lunar_MonthLeapString[Lunar.IsLeap], Lunar_MonthString[Lunar.Month],
                     Lunar_DateString[Lunar.Date], Lunar_StemStrig[LUNAR_GetStem(&Lunar)],
                     Lunar_BranchStrig[LUNAR_GetBranch(&Lunar)], Lunar_ZodiacString[LUNAR_GetZodiac(&Lunar)]);
}

static void drawWeekHeader(int16_t x, int16_t y)
{
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setBackgroundColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
    display.fillRect(x, y, 380, 18, display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
    for (int i = 0; i < 7; i++)
    {
        u8g2Fonts.setCursor(x + 15 + i * 55, y + 14);
        u8g2Fonts.print(Lunar_DayString[i]);
    }
}

static void drawMonthDay(int16_t x, int16_t y, tm_t &tm, struct Lunar_Date &Lunar, uint8_t day)
{
    if (day == tm.tm_mday)
    {
        display.fillCircle(x + 10, y + 9, 22, display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
        u8g2Fonts.setForegroundColor(GxEPD_WHITE);
        u8g2Fonts.setBackgroundColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
    }
    else
    {
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    }

    u8g2Fonts.setFont(u8g2_font_wqy12b_t_lunar);
    u8g2Fonts.setCursor(x + 2, y + 4);
    u8g2Fonts.print(day);

    u8g2Fonts.setFont(u8g2_font_wqy9_t_lunar);
    u8g2Fonts.setCursor(x, y + 24);
    uint8_t JQdate;
    if (GetJieQi(tm.tm_year + YEAR0, tm.tm_mon + 1, day, &JQdate) && JQdate == day)
    {
        uint8_t JQ = (tm.tm_mon + 1 - 1) * 2;
        if (day >= 15)
            JQ++;
        if (display.epd2.hasColor && day != tm.tm_mday)
            u8g2Fonts.setForegroundColor(GxEPD_RED);
        u8g2Fonts.print(JieQiStr[JQ]);
    }
    else
    {
        LUNAR_SolarToLunar(&Lunar, tm.tm_year + YEAR0, tm.tm_mon + 1, day);
        if (Lunar.Date == 1)
            u8g2Fonts.print(Lunar_MonthString[Lunar.Month]);
        else
            u8g2Fonts.print(Lunar_DateString[Lunar.Date]);
    }
}

void EPDDrawCalendar(uint32_t timestamp, bool partial)
{
    tm_t tm = {0};
    struct Lunar_Date Lunar;
    transformTime(timestamp, &tm);

    partial = partial && (onscreen.year == tm.tm_year) && (onscreen.month == tm.tm_mon);
    partial = partial && (partialUpdates == 0 || partialUpdates % 5 > 0);

    uint8_t firstDayWeek = get_first_day_week(tm.tm_year + YEAR0, tm.tm_mon + 1);
    uint8_t monthMaxDays = thisMonthMaxDays(tm.tm_year + YEAR0, tm.tm_mon + 1);

    if (partial && display.epd2.hasFastPartialUpdate)
    {
        display.setPartialWindow(0, 0, 400, 25);
        display.firstPage();
        do
        {
            display.fillScreen(GxEPD_WHITE);
            drawDateHeader(10, 22, tm, Lunar);
        } while (display.nextPage());

        for (uint8_t i = 0; i < monthMaxDays; i++)
        {
            if ((onscreen.day > 0 && i + 1 == onscreen.day) || i + 1 == tm.tm_mday)
            {
                display.setPartialWindow(10 + (firstDayWeek + i) % 7 * 55, 45 + (firstDayWeek + i) / 7 * 50, 45, 50);
                display.firstPage();
                do
                {
                    display.fillScreen(GxEPD_WHITE);
                    drawMonthDay(22 + (firstDayWeek + i) % 7 * 55, 60 + (firstDayWeek + i) / 7 * 50, tm, Lunar, i + 1);
                } while (display.nextPage());
            }
        }

        partialUpdates++;
    }
    else
    {
        display.setFullWindow();
        display.firstPage();
        do
        {
            display.fillScreen(GxEPD_WHITE);

            drawDateHeader(10, 22, tm, Lunar);
            drawWeekHeader(10, 26);

            for (uint8_t i = 0; i < monthMaxDays; i++)
            {
                drawMonthDay(22 + (firstDayWeek + i) % 7 * 55, 60 + (firstDayWeek + i) / 7 * 50, tm, Lunar, i + 1);
            }
        } while (display.nextPage());

        partialUpdates = 0;
    }

    onscreen.year = tm.tm_year;
    onscreen.month = tm.tm_mon;
    onscreen.day = tm.tm_mday;
}

/* The Image Transfer module sends the image of your choice to Bluefruit LE over UART.
 * Each image sent begins with
 * - A single byte char '!' (0x21) followed by 'I' helper for image
 * - Color depth: 24-bit for RGB 888, 16-bit for RGB 565
 * - Image width (uint16 little endian, 2 bytes)
 * - Image height (uint16 little endian, 2 bytes)
 * - Pixel data encoded as RGB 16/24 bit and suffixed by a single byte CRC.
 *
 * Format: [ '!' ] [ 'I' ] [uint8_t color bit] [ uint16 width ] [ uint16 height ] [ r g b ] [ r g b ] [ r g b ] … [ CRC ]
 */
bool EPDUartImage::onUartData(BLEUart *pUart)
{
    if (w == 0 && h == 0)
    {
        while (pUart->available() && pUart->read8() != '!')
            ;
        if (pUart->read8() != 'I' || !pUart->available())
            return false;

        depth = pUart->read8();
        w = pUart->read16();
        h = pUart->read16();
        pixels = 0;

        if (w == 0 || h == 0)
            return false;

        if (m_start_cb)
            m_start_cb(this);

        display.setFullWindow();
    }

    while (true)
    {
        uint8_t red, green, blue;
        bool whitish = false;
        bool colored = false;

        if (depth == 24) // 24bit rgb 888 bitmap
        {
            if (pUart->available() < 3)
                break;
            red = pUart->read8();
            green = pUart->read8();
            blue = pUart->read8();
            whitish = (red > 0x80) && (green > 0x80) && (blue > 0x80);   // whitish
            colored = (red > 0x80) || ((green > 0x80) && (blue > 0x80)); // reddish or yellowish?
        }
        else if (depth == 16) // 16bit rgb 565 bitmap
        {
            if (pUart->available() < 2)
                break;
            uint16_t c565 = pUart->read16();
            red = (c565 & 0xF800) >> 8;
            green = (c565 & 0x07E0) >> 3;
            blue = (c565 & 0x001F) << 3;
            whitish = (red > 0x80) && (green > 0x80) && (blue > 0x80);   // whitish
            colored = (red > 0x80) || ((green > 0x80) && (blue > 0x80)); // reddish or yellowish?
        }
        else if (depth == 1) // 1bit format for bw pixels (0=black, 1=white)
        {
            if (0 == in_bits)
            {
                if (!pUart->available())
                    break;
                in_byte = pUart->read8();
                in_bits = 8;
            }
            whitish = (in_byte & (1 << (in_bits - 1))) != 0;
            colored = 0;
            in_bits--;
        }
        else if (depth == 2) // 2bit format for bwr pixels (0=black, 1=white, 2=red)
        {
            if (0 == in_bits)
            {
                if (!pUart->available())
                    break;
                in_byte = pUart->read8();
                in_bits = 8;
            }
            uint8_t n = in_bits - 2;
            uint8_t bits = in_byte & (0b11 << n);
            whitish = bits == (0b01 << n);
            colored = bits == (0b10 << n);
            in_bits -= 2;
        }
        else
        {
            NIMBLE_LOGE("EPD", "incorrect image color bits");
            return false;
        }

        uint16_t x = pixels / w;
        uint16_t y = pixels % w;

        if (whitish)
        {
            // keep white
        }
        else if (colored && depth > 1)
        {
            out_color_byte &= ~(0x80 >> y % 8); // colored
        }
        else
        {
            out_byte &= ~(0x80 >> y % 8); // black
        }
        if ((7 == y % 8) || (y == w - 1)) // write that last byte! (for w%8!=0 border)
        {
            color_buffer[out_idx] = out_color_byte;
            mono_buffer[out_idx++] = out_byte;
            out_byte = 0xFF;
            out_color_byte = 0xFF;
        }

        if (pixels % w == 0)
        {
            display.writeImage(mono_buffer, color_buffer, 0, x, w, 1);
            out_idx = 0;
        }
        pixels++;
    }

    if (pixels == w * h)
    {
        if (m_end_cb)
            m_end_cb(this);
        display.refresh();
        reset();
    }

    return true;
}

void EPDUartImage::reset()
{
    w = 0;
    h = 0;
    depth = 0;
    pixels = 0;
}