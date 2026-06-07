/*
 * Address miner for Yggdrasil Network 0.4.x and higher.
 *
 * developers: Vort, acetone, R4SAS, lialh4, filarius, orignal
 * developers team, 2021-2026 (c) GPLv3
 *
 */

#include "main.h"
#include "version.h"

#ifndef _WIN32
    #include <sys/stat.h> // chmod: файл лога содержит приватные ключи
#endif

std::time_t sygstartedin = std::time(NULL); // для вывода времени работы
int countsize = 0;                          // определяет периодичность вывода счетчика
std::atomic<uint64_t> totalcount(0);        // общий счетчик
std::atomic<uint64_t> countfortune(0);      // счетчик нахождений
std::atomic<int64_t> blocks_duration_ns(0); // суммарная длительность блоков, нс
std::atomic<int> altitude(20);              // текущая высота, общая для потоков
bool newline = false;                       // форматирует вывод после нахождения адреса
std::mutex mtx;
static option conf;

void intro()
{
    // строки рамки центрируются программно, чтобы она не разъезжалась при правках
    const auto centered = [](std::string text)
    {
        const size_t width = 74; // внутренняя ширина рамки
        if ((width - text.size()) % 2) text += ' ';
        const std::string pad((width - text.size()) / 2, ' ');
        return " |" + pad + text + pad + "| \n";
    };

    std::cout << std::endl
 << " +--------------------------------------------------------------------------+ \n"
 << centered("[   SimpleYggGen C++  " SYG_VERSION_FULL "   ]")
 << centered("EdDSA public key -> IPv6 -> Meshname")
 << centered("github.com/freeacetone/SimpleYggGen")
 << centered("")
 << centered("GPLv3 (c) 2021-2026")
 << " +--------------------------------------------------------------------------+ "
 << std::endl;
}

void displayConfig()
{
    // из-за регулирования количества потоков и countsize вызов функции обязателен
    unsigned int processor_count = std::thread::hardware_concurrency(); // кол-во процессоров
    if (conf.proc == 0 || conf.proc > static_cast<unsigned int>(processor_count))
        conf.proc = static_cast<unsigned int>(processor_count);

    countsize = 80000 * conf.proc;
    altitude = conf.high; // стартовая высота для майнинг-потоков

    std::cout << " Threads: " << conf.proc << ", ";

    if(conf.mode == 0)
        std::cout << "IPv6 pattern (" << conf.str << "), ";
    else if(conf.mode == 1)
    {
        std::cout << "high addresses (2" << std::setw(2) << std::setfill('0') <<
            std::hex << conf.high << std::dec;
            (conf.letsup != 0) ? std::cout << "++), " : std::cout << "+), ";
    }
    else if(conf.mode == 2)
    {
        std::cout << "by pattern (" << conf.str << ") & high (2" <<
            std::setw(2) << std::setfill('0') << std::hex << conf.high << std::dec;
            (conf.letsup != 0) ? std::cout << "++), " : std::cout << "+), ";
    }
    else if(conf.mode == 3)
        std::cout << "IPv6 regexp (" << conf.str << "), ";
    else if(conf.mode == 4)
    {
        std::cout << "IPv6 regexp (" << conf.str << ") & high (2" <<
            std::setw(2) << std::setfill('0') << std::hex << conf.high << std::dec;
            (conf.letsup != 0) ? std::cout << "++), " : std::cout << "+), ";
    }
    else if(conf.mode == 5)
        std::cout << "meshname pattern (" << conf.str << "), ";
    else if(conf.mode == 6)
        std::cout << "meshname regexp (" << conf.str << "), ";
    else if(conf.mode == 7)
        std::cout << "subnet brute force (" << conf.str << "/" << (conf.sbt_size+1) * 8 << "), ";

    if(conf.log)
        std::cout << "logging to text file.";
    else
        std::cout << "console log only.";

    if((conf.mode == 5 || conf.mode == 6) && conf.mesh == 0)
        conf.mesh = 1; // принудительно включаем отображение мешнейм-доменов при их майнинге
    std::cout << std::endl << std::endl;
}

void testOutput()
{
    if(!conf.log) return;

    if(conf.mode == 0)
        conf.outputfile = "syg-ipv6-pattern.txt";
    else if(conf.mode == 1)
        conf.outputfile = "syg-ipv6-high.txt";
    else if(conf.mode == 2)
        conf.outputfile = "syg-ipv6-pattern-high.txt";
    else if(conf.mode == 3)
        conf.outputfile = "syg-ipv6-regexp.txt";
    else if(conf.mode == 4)
        conf.outputfile = "syg-ipv6-regexp-high.txt";
    else if(conf.mode == 5)
        conf.outputfile = "syg-meshname-pattern.txt";
    else if(conf.mode == 6)
        conf.outputfile = "syg-meshname-regexp.txt";
    else if(conf.mode == 7)
        conf.outputfile = "syg-subnet-brute-force.txt";

    std::ifstream test(conf.outputfile);
    const bool exists = static_cast<bool>(test);
    test.close();

    if(!exists)
    {
        std::ofstream output(conf.outputfile);
        output << "******************************************************\n"
               << "Change PublicKey and PrivateKey to your yggdrasil.conf\n"
               << "Windows: C:\\ProgramData\\Yggdrasil\\yggdrasil.conf\n"
               << "Debian: /etc/yggdrasil.conf\n"
               << "******************************************************\n";
        if (!output.good())
            std::cerr << " WARNING: can't create log file \"" << conf.outputfile
                      << "\", found keys will be printed to console in full format" << std::endl << std::endl;
        output.close();
    }

#ifndef _WIN32
    chmod(conf.outputfile.c_str(), S_IRUSR | S_IWUSR); // 0600: в файле приватные ключи
#endif
}

// Вызывается только потоком, чей инкремент totalcount дал кратное countsize:
// перечитывание общего счетчика здесь приводило бы к двойному входу и
// делению на почти нулевую длительность (мусорные kH/s)
void logStatistics()
{
    mtx.lock();
    auto timedays = (std::time(NULL) - sygstartedin) / 86400;
    auto timehours = ((std::time(NULL) - sygstartedin) - (timedays * 86400)) / 3600;
    auto timeminutes = ((std::time(NULL) - sygstartedin) - (timedays * 86400) - (timehours * 3600)) / 60;
    auto timeseconds = (std::time(NULL) - sygstartedin) - (timedays * 86400) - (timehours * 3600) - (timeminutes * 60);

    const double block_ms = blocks_duration_ns.exchange(0) / 1.0e6;
    const uint64_t khs = block_ms > 0 ? conf.proc * countsize / block_ms : 0;
    const uint64_t total = totalcount;
    const uint64_t found = countfortune;
    std::cout <<
        " kH/s: [" << std::setw(7) << std::setfill('_') << khs <<
        "] Total: [" << std::setw(19) << total <<
        "] Found: [" << std::setw(3) << found <<
        "] Time: [" << timedays << ":" << std::setw(2) << std::setfill('0') <<
        timehours << ":" << std::setw(2) << timeminutes << ":" << std::setw(2) << timeseconds << "]" << std::endl;
    newline = true;
    mtx.unlock();
}

void logKeys(const Address& raw, const KeysBox& keys)
{
    mtx.lock();

    bool logged = false;
    if (conf.log) // запись в файл
    {
        std::ofstream output(conf.outputfile, std::ios::app);
        if (output)
        {
            output << std::endl;
            if (conf.mesh)
                output << "Domain:     " << pickupMeshnameForOutput(getBase32(raw)) << std::endl;
            output << "Address:    " << getAddress(raw) << std::endl;
            output << "PublicKey:  " << keyToString(keys.PublicKey) << std::endl;
            output << "PrivateKey: " << keyToString(keys.PrivateKey) << keyToString(keys.PublicKey) << std::endl;
            logged = output.good();
        }
    }

    if(newline) // добавляем пустую строку на экране между счетчиком и новым адресом
    {
        std::cout << std::endl;
        newline = false;
    }
    if (conf.mesh)
        std::cout << " Domain:     " << pickupMeshnameForOutput(getBase32(raw)) << std::endl;
    std::cout << " Address:    " << getAddress(raw) << std::endl;
    std::cout << " PublicKey:  " << keyToString(keys.PublicKey) << std::endl;
    std::cout << " PrivateKey: " << keyToString(keys.PrivateKey);

    // Полный формат приватного ключа (seed + публичный, как в yggdrasil.conf) выводится
    // в консоль по запросу --full-pk, а также если ключ не удалось сохранить в файл
    if (!logged || conf.fullkeys) std::cout << keyToString(keys.PublicKey);
    std::cout << std::endl;

    if (conf.log && !logged)
        std::cerr << " WARNING: can't write to \"" << conf.outputfile
                  << "\", the key above is printed in full format" << std::endl;

    std::cout << std::endl;
    mtx.unlock();
}

bool subnetCheck() // замена 300::/64 на целевой 200::/7
{
    if(!conf.str.empty() && conf.str[0] == '3')
    {
        conf.str[0] = '2';
        return true;
    }
    return false;
}

void process_fortune_key(const KeysBox& keys)
{
    Key invKey = bitwiseInverse(keys.PublicKey);
    int ones = getOnes(invKey);
    Address rawAddr;
    getRawAddress(ones, invKey, rawAddr);
    ++countfortune;
    logKeys(rawAddr, keys);
}

template <int T>
void miner_thread()
{
    Address rawForBrute {};
    if (T == 7) // строка нормализована в startThreads() до запуска потоков
        convertStrToRaw(conf.str, rawForBrute);

    Address rawAddr {};
    std::regex regx; // проверена в main() до запуска потоков
    if (T == 3 || T == 4 || T == 6)
        regx = std::regex(conf.str, std::regex_constants::egrep | std::regex_constants::icase);
    int ones = 0;

    for (;;) // основной цикл майнинга
    {
        auto start_time = std::chrono::steady_clock::now();
        KeysBox keys = getKeyPair();
        Key invKey = bitwiseInverse(keys.PublicKey);
        ones = getOnes(invKey);

        if (T == 0) // IPv6 pattern mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (getAddress(rawAddr).find(conf.str) != std::string::npos)
            {
                process_fortune_key(keys);
            }
        }
        if (T == 1) // high mining
        {
            if (ones > altitude)
            {
                if (conf.letsup) altitude = ones;
                process_fortune_key(keys);
            }
        }
        if (T == 2) // pattern & high mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (ones > altitude && getAddress(rawAddr).find(conf.str) != std::string::npos)
            {
                if (conf.letsup) altitude = ones;
                process_fortune_key(keys);
            }
        }
        if (T == 3) // IPv6 regexp mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (std::regex_search((getAddress(rawAddr)), regx))
            {
                process_fortune_key(keys);
            }
        }
        if (T == 4) // IPv6 regexp & high mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (ones > altitude)
            {
                if (std::regex_search((getAddress(rawAddr)), regx))
                {
                    if (conf.letsup) altitude = ones;
                    process_fortune_key(keys);
                }
            }
        }
        if (T == 5) // meshname pattern mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (getBase32(rawAddr).find(conf.str) != std::string::npos)
            {
                process_fortune_key(keys);
            }
        }
        if (T == 6) // meshname regexp mining
        {
            getRawAddress(ones, invKey, rawAddr);
            if (std::regex_search((getBase32(rawAddr)), regx))
            {
                process_fortune_key(keys);
            }
        }
        if (T == 7) // subnet brute force
        {
            getRawAddress(ones, invKey, rawAddr);
            for(int z = 0; z < static_cast<int>(ADDRIPV6) && rawForBrute[z] == rawAddr[z]; ++z)
            {
                if (z > 4)
                {
                    if (z == conf.sbt_size)
                    {
                        process_fortune_key(keys);
                        break;
                    }
                    else
                    {
                        mtx.lock();
                        std::cout << " So close! Bruted bytes: " << z+1
                                  << "/" << conf.sbt_size+1 << std::endl;
                        mtx.unlock();
                    }
                }
            }
        }

        auto stop_time = std::chrono::steady_clock::now();
        const uint64_t count = ++totalcount;
        blocks_duration_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(stop_time - start_time).count();
        if (count % static_cast<uint64_t>(countsize) == 0)
            logStatistics();
    }
}

void startThreads()
{
    // Подготовка строки поиска выполняется однократно до запуска потоков:
    // конкурентная правка conf.str из потоков была бы гонкой данных
    if (conf.mode == 5)
        conf.str = pickupStringForMeshname(conf.str);

    if (conf.mode == 7)
    {
        const std::string oldString = conf.str;
        const bool edited = subnetCheck();
        Address raw {};
        const bool result = convertStrToRaw(conf.str, raw);
        if (!result)
        {
            error(-505);
            std::cerr << " String [" << oldString << "] is not convertible to IPv6 address" << std::endl;
            std::exit(-505);
        }
        if (edited || conf.str != getAddress(raw))
        {
            std::cerr << " WARNING: Your string [" << oldString << "] converted to IP [" <<
                getAddress(raw) << "]" << std::endl << std::endl;
        }
        conf.str = getAddress(raw); // каноничная форма для майнинг-потоков
    }

    std::vector<std::thread> threads;
    threads.reserve(conf.proc);
    for (unsigned int i = 0; i < conf.proc; ++i)
    {
        threads.emplace_back(
            conf.mode == 0 ? miner_thread<0> :
            conf.mode == 1 ? miner_thread<1> :
            conf.mode == 2 ? miner_thread<2> :
            conf.mode == 3 ? miner_thread<3> :
            conf.mode == 4 ? miner_thread<4> :
            conf.mode == 5 ? miner_thread<5> :
            conf.mode == 6 ? miner_thread<6> :
            miner_thread<7>
        );
    }
    for (auto& thread : threads)
        thread.join();
}

void error(int code)
{
    std::cerr << std::endl << "\
 +--------------------------------------------------------------------------+\n\
 | Incorrect input, my dear friend. Use --help for usage information.       |\n\
 +--------------------------------------------------------------------------+\n\
 Error code: " << code << std::endl;
}

void help()
{
    std::cout << std::endl << "\
 +--------------------------------------------------------------------------+\n\
 |            Simple Yggdrasil address miner usage:  --help or -h           |\n\
 +--------------------------------------------------------------------------+\n\
 [Mining modes]                                                              \n\
   High addresses                                    BY DEFAULT |            \n\
   IPv6 by pattern                                         --ip | -i         \n\
   IPv6 by pattern + height                           --ip-high | -ih        \n\
   IPv6 by regular expression                          --regexp | -r         \n\
   IPv6 by regular expression + height            --regexp-high | -rh        \n\
   Meshname by pattern                                   --mesh | -m         \n\
   Meshname by regular expression                 --mesh-regexp | -mr        \n\
   Subnet brute force (300::/64)                  --brute-force | -b         \n\
 [Main parameters]                                                           \n\
   Threads count (maximum by default)                 --threads | -t  <value>\n\
   String for pattern or regular expression           --pattern | -p  <value>\n\
   Start position for high addresses (14 by default) --altitude | -a  <value>\n\
 [Extra options]                                                             \n\
   Disable auto-increase in high mode           --increase-none | -in        \n\
   Disable logging to text file, stdout only     --logging-none | -ln        \n\
   Force display meshname domains                --display-mesh | -dm        \n\
   Show PrivateKeys in full format in console         --full-pk | -fp        \n\
   Show the version of the miner                      --version | -v         \n\
 [Meshname convertation]                                                     \n\
   Convert IP to Meshname                              --tomesh | -tm <value>\n\
   Convert Meshname to IP                                --toip | -ti <value>\n\
 [Notes]                                                                     \n\
   Meshname domains use base32 (RFC4648) alphabet symbols.                   \n\
   In meshname domain mining should use \"=\" instead \".meship\" or \".meshname\".\n\
   Subnet brute force mode understand \"3xx:\" and \"2xx:\" patterns.        \n\
 +--------------------------------------------------------------------------+\n";
}

void without()
{
    std::cout << "\
 SimpleYggGen was started without parameters.\n\
 The mining mode for high addresses will be launched automatically.\n\
 Use --help for usage information."
 << std::flush;
}

int main(int argc, char *argv[])
{
    if (!initSodium())
    {
        std::cerr << " FATAL: libsodium initialization failed" << std::endl;
        return 1;
    }

    if(argc >= 2)
    {
        std::string p1;
        ///////////////////////////////// Вспомогательные функции
        p1 = argv[1];
        if (p1 == "--help" || p1 == "-help" || p1 == "-h") {
            help();
            return 0;
        } else if (p1 == "--version" || p1 == "-v") {
            intro();
            return 0;
        } else if (p1 == "--tomesh" || p1 == "-tm") { // преобразование IP -> Meshname
            if (argc >= 3) {
                Address rawAddr {};
                if (!convertStrToRaw(argv[2], rawAddr)) {
                    error(-503);
                    std::cerr << " \"" << argv[2] << "\" is not a valid IPv6 address" << std::endl;
                    return -503;
                }
                std::cout << std::endl << pickupMeshnameForOutput(getBase32(rawAddr)) << std::endl;
                return 0;
            } else { error(-501); return -501; }
        } else if (p1 == "--toip" || p1 == "-ti") { // преобразование Meshname -> IP
            if (argc >= 3) {
                const std::string ip = decodeMeshToIP(argv[2]);
                if (ip.empty()) {
                    error(-504);
                    std::cerr << " \"" << argv[2] << "\" is not a valid meshname domain" << std::endl;
                    return -504;
                }
                std::cout << std::endl << ip << std::endl;
                return 0;
            } else { error(-502); return -502; }

        ///////////////////////////////// Основные функции
        } else {
            int res = -1;
            for(int i = 1;; ++i) {
                if (argv[i] == nullptr) break;

                res = parameters(conf, std::string(argv[i]));
                if (res == 777) { // Нужно передать параметр
                    ++i;
                    if (argv[i] == nullptr) { // Значение параметра не передано
                        error(776);
                        std::cerr << " Empty value for parameter \"" << argv[i-1] << "\"" << std::endl;
                        return 776;
                    }

                    int res2 = parameters(conf, std::string( std::string(argv[i-1]) + " " + std::string(argv[i])) );
                    if (res2 != 0) { // Значение передано, но является некорректным
                        error(res2);
                        std::cerr << " Wrong value \"" << argv[i] <<"\" for parameter \"" << argv[i-1] << "\"" << std::endl;
                        return res2;
                    }
                }
                else if (res != 0) { // Неизвестный параметр
                    error(res);
                    std::cerr << " Unknown parameter \"" << argv[i] << "\"" << std::endl;
                    return res;
                }
            }
        }
    }
    else { without(); std::this_thread::sleep_for(std::chrono::seconds(1)); }

    if (conf.mode == 3 || conf.mode == 4 || conf.mode == 6)
    {
        // Регулярное выражение валидируется до запуска потоков: исключение
        // в майнинг-потоке привело бы к std::terminate
        try {
            std::regex test(conf.str, std::regex_constants::egrep | std::regex_constants::icase);
        } catch (const std::regex_error&) {
            error(-506);
            std::cerr << " Invalid regular expression: \"" << conf.str << "\"" << std::endl;
            return -506;
        }
    }

    intro();
    displayConfig();
    testOutput();
    startThreads();
}
