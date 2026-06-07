#include "core.h"

#include <cctype>
#include <cstring>
#include <vector>

#ifdef _WIN32 // преобразование в IPv6
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
#endif

#include "cppcodec/base32_rfc4648.hpp"
#include "cppcodec/parse_error.hpp"

bool initSodium()
{
    return sodium_init() >= 0; // повторный вызов безопасен и сразу вернет 1
}

KeysBox getKeyPair()
{
    KeysBox keys;

    uint8_t sk[64];
    crypto_sign_ed25519_keypair(keys.PublicKey.data(), sk);
    memcpy(keys.PrivateKey.data(), sk, 32);

    return keys;
}

Key bitwiseInverse(const Key& key)
{
    Key inverted;
    for(size_t i = 0; i < key.size(); ++i)
        inverted[i] = ~key[i];

    return inverted;
}

int getOnes(const Key& value)
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

void getRawAddress(int lErase, Key InvertedPublicKey, Address& rawAddr)
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
        rawAddr[i + 2] = InvertedPublicKey[i+start];
}

std::string getAddress(const Address& rawAddr)
{
    char ipStrBuf[46];
    inet_ntop(AF_INET6, rawAddr.data(), ipStrBuf, 46);
    return std::string(ipStrBuf);
}

bool convertStrToRaw(const std::string& str, Address& array)
{
    return inet_pton(AF_INET6, str.c_str(), (void*)array.data());
}

std::string getBase32(const Address& rawAddr)
{
    return static_cast<std::string>(cppcodec::base32_rfc4648::encode(rawAddr.data(), ADDRIPV6));
}

std::string hexArrayToString(const uint8_t* bytes, int length)
{
    static const char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(length * 2);
    for (int i = 0; i < length; i++)
    {
        result += hex[bytes[i] >> 4];
        result += hex[bytes[i] & 0x0F];
    }
    return result;
}

std::string keyToString(const Key& key)
{
    return hexArrayToString(key.data(), KEYSIZE);
}

/**
 * pickupStringForMeshname получает человекочитаемую строку
 * типа fsdasdaklasdgdas.meship и возвращает значение, пригодное
 * для поиска по meshname-строке: удаляет возможную доменную зону
 * (всё после точки и саму точку), а также делает все буквы
 * заглавными.
 */
std::string pickupStringForMeshname(std::string str)
{
    for (auto& c : str)
        c = toupper(c); // делаем все буквы заглавными для обработки

    const size_t dot = str.find('.');
    if (dot != std::string::npos)
        str.erase(dot); // удаляем доменную зону

    return str;
}

/**
 * pickupMeshnameForOutput получает сырое base32 значение
 * типа KLASJFHASSA7979====== и возвращает meshname-домен:
 * делает все символы строчными и удаляет паддинги ('='),
 * а также добавляет доменную зону ".meship".
 */
std::string pickupMeshnameForOutput(std::string str)
{
    for (auto& c : str)
        c = tolower(c); // делаем все буквы строчными для вывода

    while (!str.empty() && str.back() == '=')
        str.pop_back(); // удаляем символы '=' в конце адреса

    return str + ".meship";
}

/**
 * decodeMeshToIP получает строковое значение сырого base32
 * кода типа KLASJFHASSA7979====== и возвращает IPv6-стринг.
 * При некорректном вводе возвращает пустую строку.
 */
std::string decodeMeshToIP(const std::string& str)
{
    try
    {
        std::string mesh = pickupStringForMeshname(str) + "======"; // 6 паддингов - норма для IPv6 адреса
        std::vector<uint8_t> raw = cppcodec::base32_rfc4648::decode(mesh);
        if (raw.size() < ADDRIPV6)
            return std::string();

        Address rawAddr;
        for(size_t i = 0; i < ADDRIPV6; ++i)
            rawAddr[i] = raw[i];
        return getAddress(rawAddr);
    }
    catch (const cppcodec::parse_error&)
    {
        return std::string();
    }
}
