/*
 * Общее ядро майнера: ключи, адреса, base32/meshname-преобразования.
 * Используется консольной (src) и графической (src-qt) версиями.
 *
 * developers: Vort, acetone, R4SAS, lialh4, filarius, orignal
 * developers team, 2021 (c) GPLv3
 *
 */

#ifndef CORE_H
#define CORE_H

#include <sodium.h>

#include <array>
#include <cstdint>
#include <string>

const size_t KEYSIZE  = 32;
const size_t ADDRIPV6 = 16;
typedef std::array<uint8_t, KEYSIZE> Key;
typedef std::array<uint8_t, ADDRIPV6> Address;

struct KeysBox
{
    Key PublicKey;
    Key PrivateKey;
};

// Обязательна до первой генерации ключей: без sodium_init()
// crypto_sign_ed25519_keypair не является потокобезопасной.
bool initSodium();

KeysBox getKeyPair();
Key bitwiseInverse(const Key& key);
int getOnes(const Key& value);
void getRawAddress(int lErase, Key InvertedPublicKey, Address& rawAddr);
std::string getAddress(const Address& rawAddr);
bool convertStrToRaw(const std::string& str, Address& array);
std::string getBase32(const Address& rawAddr);
std::string hexArrayToString(const uint8_t* bytes, int length);
std::string keyToString(const Key& key);
std::string pickupStringForMeshname(std::string str);
std::string pickupMeshnameForOutput(std::string str);
std::string decodeMeshToIP(const std::string& str); // возвращает "" при некорректном вводе

#endif // CORE_H
