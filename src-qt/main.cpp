/*
 * developers of GUI version: acetone, Vort
 * developers team, 2021-2026 (c) GPLv3
 *
 */

#include "widget.h"
#include "../src/core.h"
#include "../src/version.h"

#include <iostream>
#include <QApplication>
#include <QString>
#include <QIcon>

const QString PRODUCT_VERSION = SYG_VERSION_FULL;

int main(int argc, char *argv[])
{
    if (!initSodium())
    {
        std::cerr << "FATAL: libsodium initialization failed" << std::endl;
        return 1;
    }

    QApplication a(argc, argv);
    Widget w;
    w.setWindowTitle("syg-cpp " + PRODUCT_VERSION + " (Qt)");
    QFont defaultFont;
    defaultFont.setStyleHint(QFont::Monospace);
    a.setFont(defaultFont);

    w.setWindowIcon(QIcon(":/icon.png"));
    w.show();
    return a.exec();
}
