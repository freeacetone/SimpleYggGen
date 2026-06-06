#include "miner.h"
#include "configure.h"
#include "../src/cppcodec/base32_rfc4648.hpp"

#include <QFile>
#include <QDebug>
#include <QRegularExpression>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <sodium.h>

QMutex Miner::m_mtx;
std::time_t Miner::m_sygstartedin = std::time(NULL);
int Miner::m_countsize = 0;
std::atomic<quint64> Miner::m_totalcount (0);
std::atomic<quint64> Miner::m_countfortune (0);
std::chrono::steady_clock::duration Miner::m_blocks_duration;

Miner::Miner(Widget *w): window(w)
{
    m_countsize = 30000 * window->conf.proc; // Периодичность обновления счетчиков

    window->conf.mode == 0 ? window->conf.outputfile = "syg-ipv6-pattern.txt" :
    window->conf.mode == 1 ? window->conf.outputfile = "syg-ipv6-high.txt" :
    window->conf.mode == 2 ? window->conf.outputfile = "syg-ipv6-pattern-high.txt" :
    window->conf.mode == 3 ? window->conf.outputfile = "syg-ipv6-regexp.txt" :
    window->conf.mode == 4 ? window->conf.outputfile = "syg-ipv6-regexp-high.txt" :
    window->conf.mode == 5 ? window->conf.outputfile = "syg-meshname-pattern.txt" :
        /* 6 */      window->conf.outputfile = "syg-meshname-regexp.txt" ;

    initializeLogFile();

    if (window->conf.mode == 6)
    {
        // поиск по сырому base32, где конец - это паддинг "====".
        for (auto it = window->conf.str.begin(); it != window->conf.str.end(); ++it)
        {
            if (*it == '$') *it = '=';
        }
    }

    if (window->conf.mode == 5) // meshname pattern
    {
        window->conf.str = pickupStringForMeshname(window->conf.str);
    }

    QObject::connect(this, &Miner::setLog, window, &Widget::setLog, Qt::QueuedConnection);
    QObject::connect(this, &Miner::setAddr, window, &Widget::setAddr, Qt::QueuedConnection);
}

void Miner::dropCounters()
{
    m_sygstartedin = std::time(NULL);
    m_countsize = 0;
    m_totalcount = 0;
    m_countfortune = 0;
    m_blocks_duration = std::chrono::steady_clock::duration::zero();
}

void Miner::initializeLogFile()
{
    QFile output(window->conf.outputfile);
    if (output.exists())
    {
        return;
    }

    if (not output.open(QIODevice::WriteOnly))
    {
        qDebug() << __PRETTY_FUNCTION__ << "can't initialize output file";
        return;
    }

    output.write("******************************************************\n"
                 "Change PublicKey and PrivateKey to your yggdrasil.conf\n"
                 "Windows: C:\\ProgramData\\Yggdrasil\\yggdrasil.conf\n"
                 "Debian: /etc/yggdrasil.conf\n"
                 "******************************************************\n");

    output.close();
}

void Miner::logStatistics()
{
    if (m_totalcount % m_countsize == 0)
    {
        auto timedays = (std::time(NULL) - m_sygstartedin) / 86400;
        auto timehours = ((std::time(NULL) - m_sygstartedin) - (timedays * 86400)) / 3600;
        auto timeminutes = ((std::time(NULL) - m_sygstartedin) - (timedays * 86400) - (timehours * 3600)) / 60;
        auto timeseconds = (std::time(NULL) - m_sygstartedin) - (timedays * 86400) - (timehours * 3600) - (timeminutes * 60);

        std::chrono::duration<double, std::milli> df = m_blocks_duration;
        m_blocks_duration = std::chrono::steady_clock::duration::zero();
        quint64 khs = window->conf.proc * m_countsize / df.count();

        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << timedays << ":" << std::setw(2) << std::setfill('0')
           << timehours << ":" << std::setw(2) << timeminutes << ":" << std::setw(2) << timeseconds;

        emit setLog(QString::fromStdString(ss.str()), m_totalcount, m_countfortune, khs);
    }
}

void Miner::logKeys(const Address& raw, const KeysBox keys)
{
    QString base32 = getBase32(raw);
    if (window->conf.mode == 5 || window->conf.mode == 6) emit setAddr(pickupMeshnameForOutput(base32));
    else                                  emit setAddr(getAddress(raw));

    m_mtx.lock();

    QFile output(window->conf.outputfile);
    if (not output.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        qDebug() << __PRETTY_FUNCTION__ << "can't write log file";
        return;
    }

    const QByteArray publicKey = keyToString(keys.PublicKey).toUtf8();
    const QByteArray data = "\n"
    "Domain:     " + pickupMeshnameForOutput(base32).toUtf8() + "\n"
    "Address:    " + getAddress(raw).toUtf8() + "\n"
    "PublicKey:  " + publicKey + "\n"
    "PrivateKey: " + keyToString(keys.PrivateKey).toUtf8() + publicKey + "\n";

    output.write(data);
    output.close();

    m_mtx.unlock();
}

QString Miner::getBase32(const Address& rawAddr)
{
    return QString::fromStdString(cppcodec::base32_rfc4648::encode(rawAddr.data(), 16));
}

/**
 * pickupStringForMeshname получает человекочитаемую строку
 * типа fsdasdaklasdgdas.meship и возвращает значение, пригодное
 * для поиска по meshname-строке: удаляет возможную доменную зону
 * (всё после точки и саму точку), а также делает все буквы
 * заглавными.
 */
QString Miner::pickupStringForMeshname(QString str)
{
    str = str.toUpper();
    qsizetype dotPos = str.indexOf('.');
    if (dotPos >= 0)
    {
        str.remove(dotPos, str.size()-dotPos);
    }
    return str;
}

/**
 * pickupMeshnameForOutput получает сырое base32 значение
 * типа KLASJFHASSA7979====== и возвращает meshname-домен:
 * делает все символы строчными и удаляет паддинги ('='),
 * а также добавляет доменную зону ".meship".
 */
QString Miner::pickupMeshnameForOutput(QString str)
{
    str = str.toLower();
    str.remove('=');
    return str + ".meship";
}

QString Miner::keyToString(const Key& key)
{
    return hexArrayToString(key.data(), KEYSIZE);
}

QString Miner::hexArrayToString(const uint8_t* bytes, int length)
{
    std::stringstream ss;
    for (int i = 0; i < length; i++)
    {
        ss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(bytes[i]);
    }
    return QString::fromStdString(ss.str());
}

QString Miner::getAddress(const Address& rawAddr)
{
    char ipStrBuf[46];
    inet_ntop(AF_INET6, rawAddr.data(), ipStrBuf, 46);
    return ipStrBuf;
}

KeysBox Miner::getKeyPair()
{
    KeysBox keys;

    uint8_t sk[64];
    crypto_sign_ed25519_keypair(keys.PublicKey.data(), sk);
    memcpy(keys.PrivateKey.data(), sk, 32);

    return keys;
}

void Miner::getRawAddress(int lErase, Key InvertedPublicKey, Address& rawAddr)
{
    ++lErase; // лидирующие единицы + первый ноль

    int bitsToShift = lErase % 8;
    int start = lErase / 8;

    for(int i = start; i < start + 15; ++i)
    {
        InvertedPublicKey[i] <<= bitsToShift;
        InvertedPublicKey[i] |= (InvertedPublicKey[i + 1] >> (8 - bitsToShift));
    }

    rawAddr[0] = 0x02;
    rawAddr[1] = lErase - 1;
    for (int i = 0; i < 14; ++i)
    {
        rawAddr[i + 2] = InvertedPublicKey[i+start];
    }
}

Key Miner::bitwiseInverse(const Key& key)
{
    Key inverted;
    for(size_t i = 0; i < key.size(); ++i)
    {
        inverted[i] = ~key[i];
    }

    return inverted;
}

int Miner::getOnes(const Key& value)
{
    const int zeroBytesMap[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
    int leadOnes = 0; // кол-во лидирующих единиц

    for (int i = 0; i < 17; ++i) // 32B(ключ) - 15B(IPv6 без 0x02) = 17B(возможных лидирующих единиц)
    {
        for (int j = 0; j < 8; ++j)
        {
            if (value[i] & zeroBytesMap[j]) ++leadOnes;
            else return leadOnes;
        }
    }
    return 0; // никогда не случится
}

void Miner::run()
{
    Address rawAddr;
    const QRegularExpression regx(window->conf.str);
    int ones = 0;

    for (;;) // основной цикл майнинга
    {
        if (window->conf.stop) break;

        auto start_time = std::chrono::steady_clock::now();
        KeysBox keys = getKeyPair();
        Key invKey = bitwiseInverse(keys.PublicKey);
        ones = getOnes(invKey);

        if (window->conf.mode == 0) // IPv6 pattern mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (getAddress(rawAddr).contains(window->conf.str))
            {
                processFortuneKey(keys);
            }
        }
        if (window->conf.mode == 1) // high mining
        {
            if (ones > window->conf.high)
            {
                if (window->conf.letsup) window->conf.high = ones;
                processFortuneKey(keys);
            }
        }
        if (window->conf.mode == 2) // pattern & high mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (ones > window->conf.high and getAddress(rawAddr).contains(window->conf.str))
            {
                if (window->conf.letsup) window->conf.high = ones;
                processFortuneKey(keys);
            }
        }
        if (window->conf.mode == 3) // IPv6 regexp mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (getAddress(rawAddr).contains(regx))
            {
                processFortuneKey(keys);
            }
        }
        if (window->conf.mode == 4) // IPv6 regexp & high mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (ones > window->conf.high and getAddress(rawAddr).contains(regx))
            {
                if (window->conf.letsup) window->conf.high = ones;
                processFortuneKey(keys);
            }
        }
        if (window->conf.mode == 5) // meshname pattern mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (getBase32(rawAddr).contains(window->conf.str))
            {
                processFortuneKey(keys);
            }
        }
        if (window->conf.mode == 6) // meshname regexp mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (getBase32(rawAddr).contains(regx))
            {
                processFortuneKey(keys);
            }
        }

        auto stop_time = std::chrono::steady_clock::now();
        ++m_totalcount;
        m_blocks_duration += stop_time - start_time;
        logStatistics();
    }
}

void Miner::processFortuneKey(const KeysBox& keys)
{
    Key invKey = bitwiseInverse(keys.PublicKey);
    int ones = getOnes(invKey);
    Address rawAddr;
    getRawAddress(ones, invKey, rawAddr);
    ++m_countfortune;
    logKeys(rawAddr, keys);
    logStatistics();
}
