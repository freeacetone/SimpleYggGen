#include "miner.h"
#include "configure.h"

#include <QFile>
#include <QDebug>
#include <QElapsedTimer>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QThread>

#include <iomanip>
#include <sstream>

#ifdef Q_OS_LINUX
    // QThread::setPriority не работает для SCHED_OTHER на Linux,
    // поэтому приоритет майнинг-потоков понижается через nice
    #include <sys/resource.h>
    #include <sys/syscall.h>
    #include <unistd.h>
#endif

QMutex Miner::m_mtx;
std::time_t Miner::m_sygstartedin = std::time(NULL);
int Miner::m_countsize = 0;
std::atomic<quint64> Miner::m_totalcount (0);
std::atomic<quint64> Miner::m_countfortune (0);
std::atomic<qint64> Miner::m_blocksDurationNs (0);

Miner::Miner(Widget *w): window(w)
{
    QObject::connect(this, &Miner::setLog, window, &Widget::setLog, Qt::QueuedConnection);
    QObject::connect(this, &Miner::setAddr, window, &Widget::setAddr, Qt::QueuedConnection);
}

void Miner::dropCounters()
{
    m_sygstartedin = std::time(NULL);
    m_totalcount = 0;
    m_countfortune = 0;
    m_blocksDurationNs = 0;
}

void Miner::configure(Widget* window)
{
    m_countsize = 30000 * window->conf.proc; // Периодичность обновления счетчиков

    window->conf.mode == 0 ? window->conf.outputfile = "syg-ipv6-pattern.txt" :
    window->conf.mode == 1 ? window->conf.outputfile = "syg-ipv6-high.txt" :
    window->conf.mode == 2 ? window->conf.outputfile = "syg-ipv6-pattern-high.txt" :
    window->conf.mode == 3 ? window->conf.outputfile = "syg-ipv6-regexp.txt" :
    window->conf.mode == 4 ? window->conf.outputfile = "syg-ipv6-regexp-high.txt" :
    window->conf.mode == 5 ? window->conf.outputfile = "syg-meshname-pattern.txt" :
        /* 6 */      window->conf.outputfile = "syg-meshname-regexp.txt" ;

    initializeLogFile(window);

    if (window->conf.mode == 6)
    {
        // поиск по сырому base32, где конец - это паддинг "====".
        window->conf.str.replace('$', '=');
    }

    if (window->conf.mode == 5) // meshname pattern
    {
        window->conf.str = QString::fromStdString(
            pickupStringForMeshname(window->conf.str.toStdString()));
    }
}

void Miner::initializeLogFile(Widget* window)
{
    QFile output(window->conf.outputfile);
    if (not output.exists())
    {
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

    // файл содержит приватные ключи - ограничиваем доступ владельцем
    output.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

// Вызывается только потоком, чей инкремент m_totalcount дал кратное m_countsize:
// перечитывание общего счетчика здесь приводило бы к двойному входу и
// делению на почти нулевую длительность (мусорные kH/s)
void Miner::logStatistics()
{
    auto timedays = (std::time(NULL) - m_sygstartedin) / 86400;
    auto timehours = ((std::time(NULL) - m_sygstartedin) - (timedays * 86400)) / 3600;
    auto timeminutes = ((std::time(NULL) - m_sygstartedin) - (timedays * 86400) - (timehours * 3600)) / 60;
    auto timeseconds = (std::time(NULL) - m_sygstartedin) - (timedays * 86400) - (timehours * 3600) - (timeminutes * 60);

    const double block_ms = m_blocksDurationNs.exchange(0) / 1.0e6;
    const quint64 khs = block_ms > 0 ? window->conf.proc * m_countsize / block_ms : 0;

    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << timedays << ":" << std::setw(2) << std::setfill('0')
       << timehours << ":" << std::setw(2) << timeminutes << ":" << std::setw(2) << timeseconds;

    emit setLog(QString::fromStdString(ss.str()), m_totalcount, m_countfortune, khs);
}

void Miner::logKeys(const Address& raw, const KeysBox& keys)
{
    const bool mesh = (window->conf.mode == 5 || window->conf.mode == 6);
    const QString domain = QString::fromStdString(pickupMeshnameForOutput(getBase32(raw)));
    const QString address = QString::fromStdString(getAddress(raw));

    QMutexLocker locker(&m_mtx); // отпускает мьютекс на любом выходе из функции

    // При потоке находок (короткий паттерн) GUI обновляется не чаще раза в 100 мс,
    // чтобы очередь событий не подвешивала интерфейс; файл получает все ключи
    static QElapsedTimer addrTimer;
    if (not addrTimer.isValid() || addrTimer.elapsed() > 100)
    {
        emit setAddr(mesh ? domain : address);
        addrTimer.restart();
    }

    QFile output(window->conf.outputfile);
    if (not output.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        qDebug() << __PRETTY_FUNCTION__ << "can't write log file";
        return;
    }

    const QByteArray publicKey = QByteArray::fromStdString(keyToString(keys.PublicKey));
    QByteArray data = "\n";
    if (mesh)
        data += "Domain:     " + domain.toUtf8() + "\n";
    data += "Address:    " + address.toUtf8() + "\n"
            "PublicKey:  " + publicKey + "\n"
            "PrivateKey: " + QByteArray::fromStdString(keyToString(keys.PrivateKey)) + publicKey + "\n";

    output.write(data);
    output.close();
}

void Miner::run()
{
    // Понижаем приоритет майнинг-потока, чтобы GUI оставался отзывчивым:
    // setPriority работает на Windows, nice - на Linux
    QThread::currentThread()->setPriority(QThread::LowPriority);
#ifdef Q_OS_LINUX
    setpriority(PRIO_PROCESS, static_cast<id_t>(syscall(SYS_gettid)), 5);
#endif

    const int mode = window->conf.mode;
    const std::string pattern = window->conf.str.toStdString();
    const QRegularExpression regx = (mode == 3 || mode == 4 || mode == 6)
        ? QRegularExpression(window->conf.str) : QRegularExpression();

    Address rawAddr {};
    int ones = 0;

    for (;;) // основной цикл майнинга
    {
        if (window->conf.stop) break;

        auto start_time = std::chrono::steady_clock::now();
        KeysBox keys = getKeyPair();
        Key invKey = bitwiseInverse(keys.PublicKey);
        ones = getOnes(invKey);

        if (mode == 0) // IPv6 pattern mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (getAddress(rawAddr).find(pattern) != std::string::npos)
            {
                processFortuneKey(keys);
            }
        }
        if (mode == 1) // high mining
        {
            if (ones > window->conf.high)
            {
                if (window->conf.letsup) window->conf.high = ones;
                processFortuneKey(keys);
            }
        }
        if (mode == 2) // pattern & high mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (ones > window->conf.high and getAddress(rawAddr).find(pattern) != std::string::npos)
            {
                if (window->conf.letsup) window->conf.high = ones;
                processFortuneKey(keys);
            }
        }
        if (mode == 3) // IPv6 regexp mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (QString::fromStdString(getAddress(rawAddr)).contains(regx))
            {
                processFortuneKey(keys);
            }
        }
        if (mode == 4) // IPv6 regexp & high mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (ones > window->conf.high and QString::fromStdString(getAddress(rawAddr)).contains(regx))
            {
                if (window->conf.letsup) window->conf.high = ones;
                processFortuneKey(keys);
            }
        }
        if (mode == 5) // meshname pattern mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (getBase32(rawAddr).find(pattern) != std::string::npos)
            {
                processFortuneKey(keys);
            }
        }
        if (mode == 6) // meshname regexp mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (QString::fromStdString(getBase32(rawAddr)).contains(regx))
            {
                processFortuneKey(keys);
            }
        }

        auto stop_time = std::chrono::steady_clock::now();
        const quint64 count = ++m_totalcount;
        m_blocksDurationNs += std::chrono::duration_cast<std::chrono::nanoseconds>(stop_time - start_time).count();
        if (count % static_cast<quint64>(m_countsize) == 0)
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
    logKeys(rawAddr, keys); // счетчик Found обновится со следующим интервалом статистики
}
