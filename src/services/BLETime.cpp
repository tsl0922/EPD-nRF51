#include <Arduino.h>
#include <NimBLELog.h>
#include "BLETime.h"

#define BLE_UUID_CURRENT_TIME_SERVICE "1805"
#define BLE_UUID_CURRENT_TIME_CHAR "2A2B"

BLETime::BLETime(NimBLEServer *pServer)
{
    m_svc = pServer->createService(BLE_UUID_CURRENT_TIME_SERVICE);
    m_char = m_svc->createCharacteristic(BLE_UUID_CURRENT_TIME_CHAR);
    m_char->setCallbacks(this);
    m_update_cb = nullptr;
}

BLETime::~BLETime()
{
    if (m_svc)
        delete m_svc;
    if (m_char)
        delete m_char;
}

void BLETime::start()
{
    m_svc->start();
}

void BLETime::inc()
{
    updateTime(m_timestamp + 1);
    if (m_update_cb)
        m_update_cb(this, UpdateReason::INC);
}

void BLETime::setTime(uint32_t timestamp)
{
    updateTime(timestamp);
    if (m_update_cb)
        m_update_cb(this, UpdateReason::SET);
}

void BLETime::updateTime(uint32_t timestamp)
{
    m_timestamp = timestamp;

    tm_t tm = {0};
    uint8_t buf[7] = {0};

    transformTime(m_timestamp, &tm);
    ble_date_time_t datetime = {
        .year = (uint16_t)(tm.tm_year + YEAR0),
        .month = (uint8_t)(tm.tm_mon + 1),
        .day = tm.tm_mday,
        .hours = tm.tm_hour,
        .minutes = tm.tm_min,
        .seconds = tm.tm_sec,
    };

    ble_date_time_encode(&datetime, buf);

    m_char->setValue(buf, 7);
}

void BLETime::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo)
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
    setTime(transformTimeStruct(&tm));
}