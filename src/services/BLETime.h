#ifndef _BLE_TIME_H_
#define _BLE_TIME_H_
#include <NimBLEDevice.h>
#include <ble_util.h>
#include <lunar.h>

class BLETime : public NimBLECharacteristicCallbacks
{
public:
    enum UpdateReason
    {
        SET,
        INC,
    };
    typedef void (*update_callback_t)(BLETime *pBLETime, UpdateReason reason);

    BLETime(NimBLEServer *pServer);
    ~BLETime();

    void start();
    void setUpdateCallback(update_callback_t cb) { m_update_cb = cb; }

    void inc();
    uint32_t getTime() { return m_timestamp; }
    void getTime(tm_t *tm) { transformTime(m_timestamp, tm); }
    void setTime(uint32_t timestamp);

    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override;

private:
    NimBLEService *m_svc;
    NimBLECharacteristic *m_char;
    uint32_t m_timestamp = 1735689600;

    update_callback_t m_update_cb;
    void updateTime(uint32_t timestamp);
};

#endif