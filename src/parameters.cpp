#include "parameters.h"

int parameters(option& conf, std::string arg)
{
    const size_t spacePos = arg.find(' ');
    if (spacePos != std::string::npos) // Строка с пробелом, значит, ключ_значение
    {
        const std::string key = arg.substr(0, spacePos);
        std::istringstream ss( arg.substr(spacePos+1) ); // Поток нужен для проверки корректности и конвертации

        if (key == "--threads" || key == "-t") {
            ss >> conf.proc;
            return ss.fail() ? 1 : 0;
        }
        if (key == "--pattern" || key == "-p") {
            ss >> conf.str;
            return ss.fail() ? 1 : 0;
        }
        if (key == "--altitude" || key == "-a") {
            ss >> std::hex >> conf.high;
            return ss.fail() ? 1 : 0;
        }
        return 1;
    }

    if      (arg == "--ip"           || arg == "-i" ) conf.mode = 0;
    else if (arg == "--ip-high"      || arg == "-ih") conf.mode = 2;
    else if (arg == "--regexp"       || arg == "-r" ) conf.mode = 3;
    else if (arg == "--regexp-high"  || arg == "-rh") conf.mode = 4;
    else if (arg == "--mesh"         || arg == "-m" ) conf.mode = 5;
    else if (arg == "--mesh-regexp"  || arg == "-mr") conf.mode = 6;
    else if (arg == "--brute-force"  || arg == "-b" ) conf.mode = 7;

    else if (arg == "--increase-none" || arg == "-in") conf.letsup   = false;
    else if (arg == "--logging-none"  || arg == "-ln") conf.log      = false;
    else if (arg == "--display-mesh"  || arg == "-dm") conf.mesh     = true;
    else if (arg == "--full-pk"       || arg == "-fp") conf.fullkeys = true;

    else if (arg == "--threads"  || arg == "-t") return 777; // Параметры, требующие значение
    else if (arg == "--pattern"  || arg == "-p") return 777;
    else if (arg == "--altitude" || arg == "-a") return 777;

    else return 778; // Неизвестный параметр

    return 0;
}
