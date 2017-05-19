/**
 * \file
 * <!--
 * Copyright 2015 Develer S.r.l. (http://www.develer.com/)
 * -->
 *
 * \brief HistoryComboBox class implementation
 *
 * \author Aurelien Rainone <aurelien@develer.com>
 */

#include "historycombobox.h"
#include "settings.h"
#include <history.h>
#include <algorithm>
#include <QLineEdit>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>


HistoryComboBox::HistoryComboBox(QWidget *parent) :
    QComboBox(parent)
{
    history = new History(this);

    connect(this, static_cast<void (HistoryComboBox::*)(int)>(&HistoryComboBox::activated),
            history, &History::setCurrent);
}

void HistoryComboBox::fillList(QString current_text)
{
    QStringList hist_lines(history->getHistory());

    clear();
    addItems(hist_lines);
    setCurrentIndex(hist_lines.indexOf(current_text));
}

void HistoryComboBox::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
        case Qt::Key_Up:
            setCurrentIndex(0);
            setItemText(0, history->previous());
            fillList(lineEdit()->text());
            break;

        case Qt::Key_Down:
            setCurrentIndex(0);
            setItemText(0, history->next());
            fillList(lineEdit()->text());
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        {
            QString line = lineEdit()->text();
            if (line.length() > 0)
            {
                // don't treat empty input
                history->add(line);
                addLineToFile(line);
                fillList("");
                emit lineEntered(line);
            }
            break;
        }
        default:
            QComboBox::keyPressEvent(e);
            break;
    }
}

void HistoryComboBox::loadHistory(QString profile)
{
    QString filename = "/home/"+qgetenv("USER")+"/.cutecom/"+profile+".history";
    QFile file(filename);
    QString line;

    if(file.open (QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&file);
        while(!stream.atEnd())
        {
            line = stream.readLine();
            history->add(line);
        }

        stream.flush();
        file.close();
    }
    else {
        qDebug() << "Can't open history file for reading: " << filename;
    }
}

void HistoryComboBox::addLineToFile(QString line)
{
    QString profile = settings::getCurrentProfile();
    QString filename = "/home/"+qgetenv("USER")+"/.cutecom/"+profile+".history";

    qDebug() << "Adding line: " << line << "to: " << filename;

    QFile historyFile(filename);

    // Create path if it doesnt exist
    if (!historyFile.exists()) {
        QDir _dir;
        _dir.mkdir("/home/"+qgetenv("USER")+"/.cutecom/");
    }

    if (historyFile.open(QIODevice::ReadWrite | QIODevice::Append)) {
        QTextStream stream(&historyFile);
        stream << line << "\n";
    }
    else {
        qDebug() << "Can't open histry file: " << filename;
    }
}
