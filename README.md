### SimpleYggGen-CPP [![Download SimpleYggGen-CPP](https://img.shields.io/sourceforge/dt/simpleygggen.svg)](https://sourceforge.net/projects/simpleygggen/files/latest/download)
```
ILITA IRC:
Yggdrasil    324:71e:281a:9ed3::41    6667
General channels: #howtoygg and #ru
```

## Майнер адресов сети [Yggdrasil](https://yggdrasil-network.github.io/)
Начиная с версии 0.4.0 Yggdrasil Network использует новый алгоритм генерации IPv6-адресов. С версии **5.0** SimpleYggGen поддерживает только новый алгоритм.

#### Сборка на Linux
- Установите необходимые пакеты:

```bash
sudo apt-get install cmake git g++ libsodium-dev
```
- Клонируйте данный репозиторий:

```bash
git clone https://notabug.org/acetone/SimpleYggGen-CPP.git
cd ./SimpleYggGen-CPP
```

- Скомпилируйте приложение:

```bash
mkdir _build && cd _build
cmake -G "Unix Makefiles" ..
make
```

- Запустите бинарный файл `sygcpp`

#### Сборка на Windows в [MSYS2](https://www.msys2.org/)

- Запустите оболочку MSYS2 MinGW 64-bit
- Установите необходимые пакеты: 

```bash
pacman -S make git mingw-w64-x86_64-gcc mingw-w64-x86_64-libsodium mingw-w64-x86_64-cmake
```

- Клонируйте данный репозиторий:

```bash
git clone https://notabug.org/acetone/SimpleYggGen-CPP.git
cd ./SimpleYggGen-CPP
```

- Скомпилируйте приложение:

```bash
mkdir _build && cd _build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

- Запустите бинарный файл `sygcpp.exe`

## [Yggdrasil Network](https://yggdrasil-network.github.io/) address miner 

Starting with version 0.4.0 Yggdrasil Network uses the new IPv6 address generation algorithm. Since version **5.0**, SimpleYggGen only supports the new algorithm.

#### How build on Linux
- Install required packages: 

```bash
sudo apt-get install cmake git g++ libsodium-dev
```

- Clone this repository:

```bash
git clone https://notabug.org/acetone/SimpleYggGen-CPP.git
cd ./SimpleYggGen-CPP
```

- Compile application:

```bash
mkdir _build && cd _build
cmake -G "Unix Makefiles" ..
make
```

- Run binary file `sygcpp`


#### How build on Windows under [MSYS2](https://www.msys2.org/) shell
- Run MSYS2 MinGW 64-bit shell
- Install required packages: 

```bash
pacman -S make git mingw-w64-x86_64-gcc mingw-w64-x86_64-libsodium mingw-w64-x86_64-cmake
```

- Clone this repository:

```bash
git clone https://notabug.org/acetone/SimpleYggGen-CPP.git
cd ./SimpleYggGen-CPP
```

- Compile application:

```bash
mkdir _build && cd _build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

- Run binary file `sygcpp.exe`
