#ifndef CONFIGURE_H
#define CONFIGURE_H

#include <string>
#include "main.h"

struct option 
{
    unsigned int proc = 0;      // количество потоков
    int  mode         = 1;      // режим майнинга
    bool log          = true;   // логгирование
    int  high         = 20;     // начальная высота при майнинге: dec(20) == hex(14)
    bool letsup       = true;   // повышение высоты при нахождении
    bool mesh         = false;  // отображение meshname-доменов
    bool fullkeys     = false;  // отображение секретного ключа в консоли в полном формате
    std::string str   = "aaaa";

    std::string outputfile;
    int sbt_size   = 7;         // 64b/8 = 8B, нумерация с нуля
    bool sbt_alarm = false;     // для симпатичного вывода предупреждения
};

#endif
