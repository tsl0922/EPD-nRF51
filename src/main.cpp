#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLELog.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <ble_util.h>
#include <time.h>
#include "font.h"
#include "lunar.h"

#define GxEPD2_BW_IS_GxEPD2_BW true
#define GxEPD2_3C_IS_GxEPD2_3C true
#define IS_GxEPD(c, x) (c##x)
#define IS_GxEPD2_BW(x) IS_GxEPD(GxEPD2_BW_IS_, x)
#define IS_GxEPD2_3C(x) IS_GxEPD(GxEPD2_3C_IS_, x)

#ifndef MAX_DISPLAY_BUFFER_SIZE
#define MAX_DISPLAY_BUFFER_SIZE 800
#endif

#if IS_GxEPD2_BW(GxEPD2_DISPLAY_CLASS)
#include <GxEPD2_BW.h>
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
#elif IS_GxEPD2_3C(GxEPD2_DISPLAY_CLASS)
#include <GxEPD2_3C.h>
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#else
#error "Unsupported GxEPD2 display class"
#endif
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN));

#define BLE_DEVICE_NAME "NRF_EPD"

#define BLE_ADV_DURATION 120000 // 120s

#define BLE_UUID_CURRENT_TIME_SERVICE "1805"
#define BLE_UUID_CURRENT_TIME_CHAR "2A2B"

#define BLE_UUID_UART_SERVICE "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_UUID_UART_RX_CHAR "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_UUID_UART_TX_CHAR "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

static NimBLEServer *pServer = nullptr;
static NimBLECharacteristic *pCtsChar = nullptr;
static NimBLECharacteristic *pUartTxChar = nullptr;

static U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

static uint16_t connHandle = BLE_HS_CONN_HANDLE_NONE;
static bool advertising = true;
static bool calendarMode = true;
static bool updateCalendar = true;
static uint32_t timestamp = 1735689600;

static void sleepModeEnter()
{
    NIMBLE_LOGI("Main", "Entering deep sleep mode");
    nrf_gpio_cfg_sense_input(WAKEUP_PIN, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    systemPowerOff();
}

void drawCalendar()
{
    NIMBLE_LOGI("Calendar", "Updating...");
    uint32_t start = millis();

    tm_t tm = {0};
    struct Lunar_Date Lunar;
    transformTime(timestamp, &tm);
    uint16_t year = tm.tm_year + YEAR0;
    uint8_t week = day_of_week_get(tm.tm_mon + 1, tm.tm_mday, year);
    uint16_t highlightColor = display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK;

    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        u8g2Fonts.setCursor(10, 22);
        u8g2Fonts.setFont(u8g2_font_wqy12b_t_lunar);
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
        u8g2Fonts.printf("%d年%02d月%02d日 星期%s", year, tm.tm_mon + 1, tm.tm_mday, Lunar_DayString[week]);

        LUNAR_SolarToLunar(&Lunar, year, tm.tm_mon + 1, tm.tm_mday);
        u8g2Fonts.setFont(u8g2_font_wqy9_t_lunar);
        u8g2Fonts.setCursor(236, 22);
        u8g2Fonts.printf("农历: %s%s%s %s%s[%s]年", Lunar_MonthLeapString[Lunar.IsLeap], Lunar_MonthString[Lunar.Month],
                         Lunar_DateString[Lunar.Date], Lunar_StemStrig[LUNAR_GetStem(&Lunar)],
                         Lunar_BranchStrig[LUNAR_GetBranch(&Lunar)], Lunar_ZodiacString[LUNAR_GetZodiac(&Lunar)]);

        u8g2Fonts.setForegroundColor(GxEPD_WHITE);
        u8g2Fonts.setBackgroundColor(highlightColor);
        display.fillRect(10, 26, 380, 18, highlightColor);
        for (int i = 0; i < 7; i++)
        {
            u8g2Fonts.setCursor(25 + i * 55, 40);
            u8g2Fonts.print(Lunar_DayString[i]);
        }

        uint8_t firstDayWeek = get_first_day_week(year, tm.tm_mon + 1);
        uint8_t monthMaxDays = thisMonthMaxDays(year, tm.tm_mon + 1);
        for (int i = 0; i < monthMaxDays; i++)
        {
            if (i == tm.tm_mday - 1)
            {
                display.fillCircle(22 + (firstDayWeek + i) % 7 * 55 + 10, 60 + (firstDayWeek + i) / 7 * 50 + 9, 20, highlightColor);
                u8g2Fonts.setForegroundColor(GxEPD_WHITE);
                u8g2Fonts.setBackgroundColor(highlightColor);
            }
            else
            {
                u8g2Fonts.setForegroundColor(GxEPD_BLACK);
                u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
            }

            u8g2Fonts.setFont(u8g2_font_wqy12b_t_lunar);
            u8g2Fonts.setCursor(22 + (firstDayWeek + i) % 7 * 55 + 2, 60 + (firstDayWeek + i) / 7 * 50 + 4);
            u8g2Fonts.print(i + 1);

            u8g2Fonts.setFont(u8g2_font_wqy9_t_lunar);
            u8g2Fonts.setCursor(22 + (firstDayWeek + i) % 7 * 55, 80 + (firstDayWeek + i) / 7 * 50 + 4);
            uint8_t JQdate;
            if (GetJieQi(year, tm.tm_mon + 1, i + 1, &JQdate) && JQdate == i + 1)
            {
                uint8_t JQ = (tm.tm_mon + 1 - 1) * 2;
                if (i + 1 >= 15)
                    JQ++;
                if (display.epd2.hasColor)
                    u8g2Fonts.setForegroundColor(GxEPD_RED);
                u8g2Fonts.print(JieQiStr[JQ]);
            }
            else
            {
                LUNAR_SolarToLunar(&Lunar, year, tm.tm_mon + 1, i + 1);
                if (Lunar.Date == 1)
                    u8g2Fonts.print(Lunar_MonthString[Lunar.Month]);
                else
                    u8g2Fonts.print(Lunar_DateString[Lunar.Date]);
            }
        }
    } while (display.nextPage());

    NIMBLE_LOGI("Calendar", "Updated! Time: %lu ms", millis() - start);
}

void ClockTimerCallback(TimerHandle_t xTimer)
{
    timestamp++;

    tm_t tm = {0};
    uint8_t buf[7] = {0};
    transformTime(timestamp, &tm);
    ble_date_time_t datetime = {
        .year = (uint16_t)(tm.tm_year + YEAR0),
        .month = (uint8_t)(tm.tm_mon + 1),
        .day = tm.tm_mday,
        .hours = tm.tm_hour,
        .minutes = tm.tm_min,
        .seconds = tm.tm_sec,
    };
    ble_date_time_encode(&datetime, buf);
    pCtsChar->setValue(buf, 7);

    if (datetime.hours == 0 && datetime.minutes == 0 && datetime.seconds == 0)
    {
        updateCalendar = true;
    }
}

class CtsCharCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        NimBLEAttValue value = pCharacteristic->getValue();
        if (value.length() < 7)
            return;
        ble_date_time_t datetime;
        ble_date_time_decode(&datetime, value.data());
        tm_t tm = {
            .tm_year = datetime.year,
            .tm_mon = datetime.month,
            .tm_mday = datetime.day,
            .tm_hour = datetime.hours,
            .tm_min = datetime.minutes,
            .tm_sec = datetime.seconds,
        };
        timestamp = transformTimeStruct(&tm);
        NIMBLE_LOGI("CtsService", "Timestamp: %lu, %04d-%02d-%02d %02d:%02d:%02d", timestamp,
                    datetime.year, datetime.month, datetime.day, datetime.hours, datetime.minutes, datetime.seconds);
        updateCalendar = true;
    }
} ctsCharCallbacks;

static void handleCmd(String cmd)
{
    cmd.trim();
    if (cmd == "clear")
    {
        calendarMode = false;
        display.setFullWindow();
        display.writeScreenBuffer();
        display.refresh();
    }
    else if (cmd == "sleep")
    {
        sleepModeEnter();
    }
    else if (cmd == "reboot")
    {
        systemRestart();
    }
    else
    {
        NIMBLE_LOGW("UartService", "Unknown command: %s", cmd.c_str());
        pUartTxChar->notify("Invalid cmd:" + cmd, connHandle);
    }
}

class UartRxCharCallbacks : public NimBLECharacteristicCallbacks
{
public:
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        NimBLEAttValue value = pCharacteristic->getValue();
        for (uint16_t i = 0; i < value.length(); i++)
            rbuf.store_char(value[i]);

        if (transferImage())
            return;

        handleCmd(static_cast<String>(value));
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
    bool transferImage()
    {
        if (w == 0 && h == 0)
        {
            while (rbuf.available() && rbuf.read_char() != '!')
                ;
            if (rbuf.read_char() != 'I' || !rbuf.available())
                return false;

            depth = rbuf.read_char();
            w = read16();
            h = read16();
            idx = 0;

            if (w == 0 || h == 0)
                return false;

            start = millis();
            calendarMode = false;
            NIMBLE_LOGI("UartService", "image: %dx%d, depth: %d", w, h, depth);
        }

        while (true)
        {
            uint8_t red, green, blue;
            bool whitish = false;
            bool colored = false;

            if (depth == 24) // 24bit rgb 888 bitmap
            {
                if (rbuf.available() < 3)
                    break;
                red = rbuf.read_char();
                green = rbuf.read_char();
                blue = rbuf.read_char();
                whitish = (red > 0x80) && (green > 0x80) && (blue > 0x80);   // whitish
                colored = (red > 0x80) || ((green > 0x80) && (blue > 0x80)); // reddish or yellowish?
            }
            else if (depth == 16) // 16bit rgb 565 bitmap
            {
                if (rbuf.available() < 2)
                    break;
                uint16_t c565 = read16();
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
                    if (!rbuf.available())
                        break;
                    in_byte = rbuf.read_char();
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
                    if (!rbuf.available())
                        break;
                    in_byte = rbuf.read_char();
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
                NIMBLE_LOGE("UartService", "incorrect color bits");
                return false;
            }

            uint16_t x = idx / w;
            uint16_t y = idx % w;

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

            if (idx % w == 0)
            {
                display.writeImage(mono_buffer, color_buffer, 0, x, w, 1);
                out_idx = 0;
            }
            idx++;
        }

        if (idx == w * h)
        {
            uint32_t end = millis();
            NIMBLE_LOGI("UartService", "Image received: %ld pixels, time: %lu us", idx, end - start);
            display.refresh();
            NIMBLE_LOGI("UartService", "Image displayed, time: %lu ms", millis() - end);
            reset();
        }

        return true;
    }

    void reset()
    {
        w = 0;
        h = 0;
        depth = 0;
        idx = 0;
    }

    inline uint16_t read16()
    {
        uint16_t num;
        uint8_t *p = (uint8_t *)&num;
        p[0] = rbuf.read_char();
        p[1] = rbuf.read_char();
        return num;
    }

private:
    uint16_t w = 0;
    uint16_t h = 0;
    uint8_t depth = 0;
    uint32_t idx = 0;
    uint32_t start = 0; // start time

    uint8_t in_byte = 0; // for depth <= 8
    uint8_t in_bits = 0; // for depth <= 8

    uint8_t out_byte = 0xFF;
    uint8_t out_color_byte = 0xFF;
    uint32_t out_idx = 0;

    uint8_t mono_buffer[GxEPD2_DRIVER_CLASS::WIDTH / 8];
    uint8_t color_buffer[GxEPD2_DRIVER_CLASS::WIDTH / 8];

    RingBuffer rbuf;
} uartRxCharCallbacks;

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        NIMBLE_LOGI("ServerCallbacks", "Client address: %s", connInfo.getAddress().toString().c_str());
        connHandle = connInfo.getConnHandle();
        pServer->updateConnParams(connInfo.getConnHandle(), 6, 6, 0, 180);
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        NIMBLE_LOGI("ServerCallbacks", "Client disconnected - start advertising");
        connHandle = BLE_HS_CONN_HANDLE_NONE;
        uartRxCharCallbacks.reset();
        advertising = true;
    }
} serverCallbacks;

void advertisingCompleteCallback(NimBLEAdvertising *pAdvert)
{
    NIMBLE_LOGI("NimBLE", "Advertising Complete");
    if (calendarMode)
    {
        pinMode(WAKEUP_PIN, INPUT_PULLDOWN);
        attachInterrupt(digitalPinToInterrupt(WAKEUP_PIN), []
                        {
            detachInterrupt(digitalPinToInterrupt(WAKEUP_PIN));
            advertising = true; }, RISING);
    }
    else
    {
        sleepModeEnter();
    }
}

void setup(void)
{
#if CONFIG_NIMBLE_CPP_LOG_LEVEL > 0
#ifdef UART_TX_PIN
    Serial.setPins(0, UART_TX_PIN);
#endif
    Serial.begin(115200);
#endif

    SPI.setPins(0, SPI_SCK_PIN, SPI_MOSI_PIN);
#ifdef EPD_BS_PIN
    pinMode(EPD_BS_PIN, OUTPUT);
    digitalWrite(EPD_BS_PIN, 0); // Set BS LOW
#endif

    display.init();
    u8g2Fonts.begin(display);

    TimerHandle_t clockTimer = xTimerCreate("ClockTimer", pdMS_TO_TICKS(1000), pdTRUE, NULL, ClockTimerCallback);
    if (clockTimer != NULL)
    {
        xTimerStart(clockTimer, 0);
    }

    NIMBLE_LOGI("NimBLE", "Starting Server");

    NimBLEDevice::init(BLE_DEVICE_NAME);
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    NimBLEService *pCtsSvc = pServer->createService(BLE_UUID_CURRENT_TIME_SERVICE);
    pCtsChar = pCtsSvc->createCharacteristic(BLE_UUID_CURRENT_TIME_CHAR);
    pCtsChar->setCallbacks(&ctsCharCallbacks);

    NimBLEService *pUartSvc = pServer->createService(BLE_UUID_UART_SERVICE);
    NimBLECharacteristic *pUartRxChar = pUartSvc->createCharacteristic(BLE_UUID_UART_RX_CHAR, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    pUartTxChar = pUartSvc->createCharacteristic(BLE_UUID_UART_TX_CHAR, NIMBLE_PROPERTY::NOTIFY);

    pUartRxChar->setCallbacks(&uartRxCharCallbacks);

    pCtsSvc->start();
    pUartSvc->start();

    const uint8_t *pAddr = NimBLEDevice::getAddress().getVal();
    char deviceName[20];
    snprintf(deviceName, sizeof(deviceName), "%s_%02X%02X", BLE_DEVICE_NAME, pAddr[1], pAddr[0]);

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName(deviceName);
    pAdvertising->addServiceUUID(pCtsSvc->getUUID());
    pAdvertising->enableScanResponse(false);
    pAdvertising->setAdvertisingCompleteCallback(advertisingCompleteCallback);
}

void loop()
{
    if (advertising)
    {
        advertising = false;
        NimBLEDevice::startAdvertising(BLE_ADV_DURATION);
        NIMBLE_LOGI("NimBLE", "Advertising Started");
    }

    if (updateCalendar)
    {
        updateCalendar = false;
        calendarMode = true;
        drawCalendar();
    }

    delay(1000);
}