#ifndef MINER_H
#define MINER_H

#include <QObject>
#include <QRunnable>
#include <QMutex>

#include <array>
#include <chrono>

#ifdef _WIN32
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
#endif

#include "widget.h"

const size_t KEYSIZE = 32;
const size_t ADDRIPV6 = 16;
typedef std::array<uint8_t, KEYSIZE> Key;
typedef std::array<uint8_t, ADDRIPV6> Address;

struct KeysBox
{
    Key PublicKey;
    Key PrivateKey;
};

class Miner : public QObject, public QRunnable
{
    Q_OBJECT

public:
    Miner(Widget* w = 0);
    static void dropCounters();

signals:
    void setLog(QString, quint64, quint64, quint64);
    void setAddr(QString);

private:
    Widget *window;

    void initializeLogFile();
    void logStatistics();
    void logKeys(const Address& raw, const KeysBox keys);
    QString getBase32(const Address& rawAddr);
    QString pickupStringForMeshname(QString str);
    QString pickupMeshnameForOutput(QString str);
    QString keyToString(const Key& key);
    QString hexArrayToString(const uint8_t* bytes, int length);
    QString getAddress(const Address& rawAddr);
    KeysBox getKeyPair();
    void getRawAddress(int lErase, Key InvertedPublicKey, Address& rawAddr);
    Key bitwiseInverse(const Key& key);
    int getOnes(const Key& value);
    void processFortuneKey(const KeysBox& keys);

    static std::time_t m_sygstartedin; // для вывода времени работы
    static int m_countsize;            // определяет периодичность вывода счетчика
    static std::atomic<quint64> m_totalcount;       // общий счетчик
    static std::atomic<quint64> m_countfortune;     // счетчик нахождений
    static std::chrono::steady_clock::duration m_blocks_duration;

    static QMutex m_mtx;

protected:
    void run() override;
};

#endif // MINER_H
