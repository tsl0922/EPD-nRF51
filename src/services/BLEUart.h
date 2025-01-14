#ifndef _BLE_UART_H_
#define _BLE_UART_H_
#include <NimBLEDevice.h>

class BLEUart : public NimBLECharacteristicCallbacks
{
public:
    typedef void (*rx_callback_t)(BLEUart *pBLEUart, NimBLEAttValue &value);

    BLEUart(NimBLEServer *pServer);
    ~BLEUart();

    void start();

    void setRxCallback(rx_callback_t cb) { m_rx_cb = cb; }

    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override;
    void onStatus(NimBLECharacteristic *pCharacteristic, int code) override;
    void onSubscribe(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue) override;

    int read() { return read8(); }
    uint8_t read8();
    uint16_t read16(void);
    uint32_t read32(void);
    int read(uint8_t *buf, size_t size);

    size_t write(const uint8_t *buf, size_t len);
    size_t write(String s) { return write((uint8_t *)s.c_str(), s.length()); }

    int available() { return m_rx_buf->available(); }
    int peek() { return m_rx_buf->peek(); }

private:
    NimBLEService *m_svc;
    NimBLECharacteristic *m_txd;
    NimBLECharacteristic *m_rxd;
    uint16_t m_conn_handle;
    uint16_t m_MTU;

    rx_callback_t m_rx_cb;

    RingBuffer *m_rx_buf;
};

#endif