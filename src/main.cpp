#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLELog.h>
#include "services/BLETime.h"
#include "services/BLEUart.h"
#include "EPD.h"

#define BLE_DEVICE_NAME "NRF_EPD"
#define BLE_ADV_DURATION 120000 // 120s

enum EPDMode
{
    NONE,
    CALENDAR,
    IMAGE,
};

static NimBLEServer *pServer = nullptr;
static BLETime *pTime = nullptr;
static BLEUart *pUart = nullptr;
static EPDUartImage *pUartImage = nullptr;

static bool advertising = true;
static bool updateCalendar = true;
static EPDMode epdMode = NONE;

static void sleepModeEnter()
{
    NIMBLE_LOGI("Main", "Entering deep sleep mode");
    nrf_gpio_cfg_sense_input(WAKEUP_PIN, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    systemPowerOff();
}

static void handleCmd(String cmd)
{
    cmd.trim();
    if (cmd == "clear")
    {
        epdMode = NONE;
        EPDClear();
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
        NIMBLE_LOGW("Uart", "Unknown cmd: %s", cmd.c_str());
        pUart->write("Unknown cmd: " + cmd);
    }
}

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        NIMBLE_LOGI("NimBLE", "Client address: %s", connInfo.getAddress().toString().c_str());
        pServer->updateConnParams(connInfo.getConnHandle(), 6, 6, 0, 180);
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        NIMBLE_LOGI("NimBLE", "Client disconnected - start advertising");
        pUartImage->reset();
        advertising = true;
    }
} serverCallbacks;

static void onAdvertisingComplete(NimBLEAdvertising *pAdvert)
{
    NIMBLE_LOGI("NimBLE", "Advertising Complete");
    if (epdMode == CALENDAR)
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

static void advertisingInit()
{
    const uint8_t *pAddr = NimBLEDevice::getAddress().getVal();
    char deviceName[20];
    snprintf(deviceName, sizeof(deviceName), "%s_%02X%02X", BLE_DEVICE_NAME, pAddr[1], pAddr[0]);

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName(deviceName);
    pAdvertising->enableScanResponse(false);
    pAdvertising->setAdvertisingCompleteCallback(onAdvertisingComplete);
}

static void advertisingStart()
{
    NimBLEDevice::startAdvertising(BLE_ADV_DURATION);
    NIMBLE_LOGI("NimBLE", "Advertising Started");
}

static void onTimeUpdate(BLETime *pBLETime, BLETime::UpdateReason reason)
{
    tm_t tm = {0};
    pBLETime->getTime(&tm);

    if (reason == BLETime::UpdateReason::SET)
    {
        NIMBLE_LOGI("EPD", "setTime: %04d-%02d-%02d %02d:%02d:%02d",
                    tm.tm_year + YEAR0, tm.tm_mon + 1, tm.tm_mday,
                    tm.tm_hour, tm.tm_min, tm.tm_sec);
        updateCalendar = true;
    }
    else if (reason == BLETime::UpdateReason::INC && epdMode == CALENDAR)
    {
        if (tm.tm_hour == 0 && tm.tm_min == 0 && tm.tm_sec == 0)
            updateCalendar = true;
    }
}

static void onUartRX(BLEUart *pBLEUart, NimBLEAttValue &value)
{
    if (pUartImage->onUartData(pBLEUart))
        return;

    handleCmd(static_cast<String>(value));
}

static void onImageStart(EPDUartImage *pi)
{
    NIMBLE_LOGI("EPD", "image: %dx%d, depth: %d", pi->w, pi->h, pi->depth);
    epdMode = IMAGE;
}

static void onImageEnd(EPDUartImage *pi)
{
    NIMBLE_LOGI("EPD", "image received, %ld pixels", pi->pixels);
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

    EPDInit();

    TimerHandle_t clockTimer = xTimerCreate(
        "ClockTimer",
        pdMS_TO_TICKS(1000),
        pdTRUE, NULL,
        [](TimerHandle_t xTimer)
        {
            pTime->inc();
        });

    if (clockTimer != NULL)
    {
        xTimerStart(clockTimer, 0);
    }

    NimBLEDevice::init(BLE_DEVICE_NAME);
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    pTime = new BLETime(pServer);
    pUart = new BLEUart(pServer);
    pUartImage = new EPDUartImage();

    pTime->setUpdateCallback(onTimeUpdate);
    pUart->setRxCallback(onUartRX);
    pUartImage->setStartCallback(onImageStart);
    pUartImage->setEndCallback(onImageEnd);

    pTime->start();
    pUart->start();

    advertisingInit();
}

void loop()
{
    if (advertising)
    {
        advertising = false;
        advertisingStart();
    }

    if (updateCalendar)
    {
        updateCalendar = false;
        bool partial = epdMode == CALENDAR;
        epdMode = CALENDAR;

        NIMBLE_LOGI("EPD", "updating calendar...");
        uint32_t start = millis();
        EPDDrawCalendar(pTime->getTime(), partial);
        NIMBLE_LOGI("EPD", "calendar updated, time: %lu ms", millis() - start);
    }

    delay(100);
}