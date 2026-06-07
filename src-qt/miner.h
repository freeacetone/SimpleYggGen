#ifndef MINER_H
#define MINER_H

#include <QObject>
#include <QRunnable>
#include <QMutex>

#include <atomic>
#include <chrono>
#include <ctime>

#include "../src/core.h"
#include "widget.h"

class Miner : public QObject, public QRunnable
{
    Q_OBJECT

public:
    Miner(Widget* w = nullptr);
    static void dropCounters();
    // Однократная подготовка перед запуском пула потоков:
    // выбор файла лога, нормализация строки поиска
    static void configure(Widget* window);

signals:
    void setLog(QString, quint64, quint64, quint64);
    void setAddr(QString);

private:
    Widget *window;

    static void initializeLogFile(Widget* window);
    void logStatistics();
    void logKeys(const Address& raw, const KeysBox& keys);
    void processFortuneKey(const KeysBox& keys);

    static std::time_t m_sygstartedin; // для вывода времени работы
    static int m_countsize;            // определяет периодичность вывода счетчика
    static std::atomic<quint64> m_totalcount;   // общий счетчик
    static std::atomic<quint64> m_countfortune; // счетчик нахождений
    static std::atomic<qint64> m_blocksDurationNs; // суммарная длительность блоков, нс

    static QMutex m_mtx;

protected:
    void run() override;
};

#endif // MINER_H
