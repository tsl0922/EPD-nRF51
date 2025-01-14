#include <Arduino.h>
#include <NimBLELog.h>
#include "BLEUart.h"

#define BLE_UUID_UART_SERVICE "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_UUID_UART_RX_CHAR "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_UUID_UART_TX_CHAR "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEUart::BLEUart(NimBLEServer *pServer)
{
    m_svc = pServer->createService(BLE_UUID_UART_SERVICE);

    m_rxd = m_svc->createCharacteristic(BLE_UUID_UART_RX_CHAR, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    m_txd = m_svc->createCharacteristic(BLE_UUID_UART_TX_CHAR, NIMBLE_PROPERTY::NOTIFY);
    m_rxd->setCallbacks(this);
    m_txd->setCallbacks(this);

    m_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    m_MTU = BLE_ATT_MTU_DFLT;

    m_rx_buf = new RingBuffer();
    m_rx_cb = nullptr;
}

BLEUart::~BLEUart()
{
    delete m_svc;
    delete m_txd;
    delete m_rxd;
    delete m_rx_buf;
}

void BLEUart::start()
{
    m_svc->start();
}

void BLEUart::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo)
{
    NimBLEAttValue value = pCharacteristic->getValue();

    for (uint16_t i = 0; i < value.length(); i++)
    {
        m_rx_buf->store_char(value[i]);
        if (m_rx_buf->isFull())
            NIMBLE_LOGW("Uart", "RX buffer is full!");
    }

    if (m_rx_cb)
        m_rx_cb(this, value);
}

void BLEUart::onStatus(NimBLECharacteristic *pCharacteristic, int code)
{
    NIMBLE_LOGI("Uart", "Status code: %d", code);
}

void BLEUart::onSubscribe(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue)
{
    if (subValue == 0)
    {
        m_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        m_MTU = BLE_ATT_MTU_DFLT;
    }
    else
    {
        m_conn_handle = connInfo.getConnHandle();
        m_MTU = connInfo.getMTU();
    }
}

uint8_t BLEUart::read8()
{
    return m_rx_buf->read_char();
}

uint16_t BLEUart::read16(void)
{
    uint16_t num;
    return read((uint8_t *)&num, sizeof(num)) ? num : 0;
}

uint32_t BLEUart::read32(void)
{
    uint32_t num;
    return read((uint8_t *)&num, sizeof(num)) ? num : 0;
}

int BLEUart::read(uint8_t *buf, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (!m_rx_buf->available())
            return i;
        buf[i] = m_rx_buf->read_char();
    }
    return size;
}

size_t BLEUart::write(const uint8_t *buf, size_t len)
{
    if (m_conn_handle == BLE_HS_CONN_HANDLE_NONE)
        return 0;

    m_txd->setValue(buf, len);
    m_txd->notify(m_conn_handle);
    return len;
}
