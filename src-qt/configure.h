#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <QString>

#include <atomic>

struct option
{
    unsigned int proc = 0;       // количество потоков
    int  mode         = 1;       // режим майнинга
    std::atomic<int> high {20};  // высота: читается и повышается майнинг-потоками
    bool letsup       = true;    // повышение высоты при нахождении
    QString str       = "aaaa";
    std::atomic<bool> stop {false}; // флаг остановки: пишет GUI-поток, читают майнеры

    QString outputfile;
};

#endif // CONFIG_HPP
