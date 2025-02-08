#include "EPD_Test.h"
#include "EPD_driver.h"

const unsigned char EPD_4IN2_4Gray_lut_vcom[] =
{
    0x00 ,0x0A ,0x00 ,0x00 ,0x00 ,0x01,
    0x60 ,0x14 ,0x14 ,0x00 ,0x00 ,0x01,
    0x00 ,0x14 ,0x00 ,0x00 ,0x00 ,0x01,
    0x00 ,0x13 ,0x0A ,0x01 ,0x00 ,0x01,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
};

const unsigned char EPD_4IN2_4Gray_lut_ww[] ={
    0x40 ,0x0A ,0x00 ,0x00 ,0x00 ,0x01,
    0x90 ,0x14 ,0x14 ,0x00 ,0x00 ,0x01,
    0x10 ,0x14 ,0x0A ,0x00 ,0x00 ,0x01,
    0xA0 ,0x13 ,0x01 ,0x00 ,0x00 ,0x01,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};

const unsigned char EPD_4IN2_4Gray_lut_bw[] ={
    0x40 ,0x0A ,0x00 ,0x00 ,0x00 ,0x01,
    0x90 ,0x14 ,0x14 ,0x00 ,0x00 ,0x01,
    0x00 ,0x14 ,0x0A ,0x00 ,0x00 ,0x01,
    0x99 ,0x0C ,0x01 ,0x03 ,0x04 ,0x01,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};

const unsigned char EPD_4IN2_4Gray_lut_wb[] ={
    0x40 ,0x0A ,0x00 ,0x00 ,0x00 ,0x01,
    0x90 ,0x14 ,0x14 ,0x00 ,0x00 ,0x01,
    0x00 ,0x14 ,0x0A ,0x00 ,0x00 ,0x01,
    0x99 ,0x0B ,0x04 ,0x04 ,0x01 ,0x01,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};

const unsigned char EPD_4IN2_4Gray_lut_bb[] ={
    0x80 ,0x0A ,0x00 ,0x00 ,0x00 ,0x01,
    0x90 ,0x14 ,0x14 ,0x00 ,0x00 ,0x01,
    0x20 ,0x14 ,0x0A ,0x00 ,0x00 ,0x01,
    0x50 ,0x13 ,0x01 ,0x00 ,0x00 ,0x01,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};

static void write_4gray_lut(epd_driver_t *driver)
{
	unsigned int count;

    driver->send_command(0x20);							//vcom
    for(count=0;count<42;count++)
        {driver->send_byte(EPD_4IN2_4Gray_lut_vcom[count]);}
    
    driver->send_command(0x21);							//red not use
    for(count=0;count<42;count++)
        {driver->send_byte(EPD_4IN2_4Gray_lut_ww[count]);}

    driver->send_command(0x22);							//bw r
    for(count=0;count<42;count++)
        {driver->send_byte(EPD_4IN2_4Gray_lut_bw[count]);}

    driver->send_command(0x23);							//wb w
    for(count=0;count<42;count++)
        {driver->send_byte(EPD_4IN2_4Gray_lut_wb[count]);}

    driver->send_command(0x24);							//bb b
    for(count=0;count<42;count++)
        {driver->send_byte(EPD_4IN2_4Gray_lut_bb[count]);}

    driver->send_command(0x25);							//vcom
    for(count=0;count<42;count++)
        {driver->send_byte(EPD_4IN2_4Gray_lut_ww[count]);}
}

static void drawNormal(epd_driver_t *driver)
{
    UWORD Width, Height;
    Width = (driver->width % 8 == 0)? (driver->width / 8 ): (driver->width / 8 + 1);
    Height = driver->height;

    driver->send_command(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            driver->send_byte(0xFF);
        }
    }

    driver->send_command(0x13);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            driver->send_byte(((j + 1)*2 > Height) ? 0x00 : 0xFF);
        }
    }

    driver->display();
}

static void draw4Gray(epd_driver_t *driver)
{
    UWORD Width, Height;
    Width = (driver->width % 8 == 0)? (driver->width / 8 ): (driver->width / 8 + 1);
    Height = driver->height;

    driver->send_command(0x10);
    for (UWORD i = 0; i < Width * Height; i++) {
        UWORD idx = (i % 50) / 12;
        if (idx > 3) idx = 3;
        if (idx == 0 || idx == 1) {
            driver->send_byte(0x00);
        } else if (idx == 2 || idx == 3) {
            driver->send_byte(0xFF);
        }
    }

    driver->send_command(0x13);
    for (UWORD i = 0; i < Width * Height; i++) {
        UWORD idx = (i % 50) / 12;
        if (idx > 3) idx = 3;
        if (idx == 0 || idx == 2) {
            driver->send_byte(0x00);
        } else if (idx == 1 || idx == 3) {
            driver->send_byte(0xFF);
        }
    }

	// Load LUT from register
    driver->send_command(0x00);
    driver->send_byte(0x3f);
	
    write_4gray_lut(driver);
	driver->display();
	
	// Load LUT from OTP
    driver->send_command(0x00);
    driver->send_byte(0x1f);
}

void EPD_4in2_test(void)
{
	epd_driver_t *driver = epd_driver_by_id(EPD_DRIVER_4IN2);

	DEV_Module_Init();

	driver->init();
	driver->clear();
    DEV_Delay_ms(500);

    drawNormal(driver);
    DEV_Delay_ms(1000);

    draw4Gray(driver);
    DEV_Delay_ms(1000);

	driver->sleep();
    DEV_Delay_ms(500);
    DEV_Module_Exit();
}

