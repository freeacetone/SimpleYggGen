#ifndef MAIN_H
#define MAIN_H

#include <atomic>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>

#include "core.h"
#include "parameters.h"

void displayConfig();
void testOutput();
void logStatistics();
void logKeys(const Address& raw, const KeysBox& keys);
bool subnetCheck();
void process_fortune_key(const KeysBox& block);
void startThreads();
template <int T>
void miner_thread();
void error(int code);
void help();
void without();

#endif // MAIN_H
