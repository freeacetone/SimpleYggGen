/*
 * developers of GUI version: acetone, Vort
 * developers team, 2021 (c) GPLv3
 *
 */

#include "widget.h"

#include <fstream>
#include <QApplication>
#include <QString>
#include <QIcon>

const QString PRODUCT_VERSION = "5.2 forkup";

int main(int argc, char *argv[])
{
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
