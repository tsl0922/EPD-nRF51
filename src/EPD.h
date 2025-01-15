#ifndef _EPD_H_
#define _EPD_H_
#include <U8g2_for_Adafruit_GFX.h>
#include "services/BLEUart.h"

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

void EPDInit();
void EPDClear();
void EPDDrawCalendar(uint32_t timestamp, bool partial = false);

class EPDUartImage
{
public:
    typedef void (*image_callback_t)(EPDUartImage *pi);

    bool onUartData(BLEUart *pUart);
    void reset();

    void setStartCallback(image_callback_t cb) { m_start_cb = cb; }
    void setEndCallback(image_callback_t cb) { m_end_cb = cb; }

    uint16_t w = 0;
    uint16_t h = 0;
    uint8_t depth = 0;
    uint32_t pixels = 0;

private:
    uint8_t in_byte = 0; // for depth <= 8
    uint8_t in_bits = 0; // for depth <= 8

    uint8_t out_byte = 0xFF;
    uint8_t out_color_byte = 0xFF;
    uint32_t out_idx = 0;

    uint8_t mono_buffer[GxEPD2_DRIVER_CLASS::WIDTH / 8];
    uint8_t color_buffer[GxEPD2_DRIVER_CLASS::WIDTH / 8];

    image_callback_t m_start_cb;
    image_callback_t m_end_cb;
};

#endif