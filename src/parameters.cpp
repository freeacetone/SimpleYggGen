#include "parametes.h"

int parameters(option& conf, std::string arg)
{
    if (arg.find(" ") != std::string::npos) // Строка с пробелом, значит, ключ_значение
    {
        const size_t npos = -1; // std::string::npos с блэкджэком и путанами

        int position = arg.find(" ");
        std::istringstream ss( arg.substr(position+1) ); // Поток нужен для проверки корректности и конвертации

        if (arg.find("--threads") != npos || arg.find("-t") != npos) {
            ss >> conf.proc;
            if (ss.fail()) return 1;
            return 0;
        }
        if (arg.find("--pattern") != npos || arg.find("-p") != npos) {
            ss >> conf.str;
            if (ss.fail()) return 1;
            return 0;
        }
        if (arg.find("--altitude") != npos || arg.find("-a") != npos) {
            ss >> std::hex >> conf.high;
            if (ss.fail()) return 1;
            return 0;
        }
    }

    else if (arg == "--ip"           || arg == "-i" ) conf.mode = 0;
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

    return 0;
}
