/**
 * \file
 * <!--
 * Copyright 2015 Develer S.r.l. (http://www.develer.com/)
 * -->
 *
 * \brief serial-ninja entry point
 *
 * \author Aurelien Rainone <aurelien@develer.com>
 */

#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Cutecom");
    QCoreApplication::setApplicationName("Settings");
    a.setStyle(QStyleFactory::create("Fusion"));
    MainWindow w;
    w.show();

    return a.exec();
}
