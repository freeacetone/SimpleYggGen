#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <QString>

struct option
{
    unsigned int proc = 0;      // количество потоков
    int  mode         = 1;      // режим майнинга
    int  high         = 20;     // начальная высота при майнинге: dec(20) == hex(14)
    bool letsup       = true;   // повышение высоты при нахождении
    QString str       = "aaaa";
    bool stop         = false;  // флаг остановки

    QString outputfile;
};

#endif // CONFIG_HPP
