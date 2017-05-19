/**
 * \file
 * <!--
 * Copyright 2015 Develer S.r.l. (http://www.develer.com/)
 * -->
 *
 * \brief Serial port connection and settings dialog
 *
 * \author Aurelien Rainone <aurelien@develer.com>
 */

#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class ConnectDialog;
}


/**
 * \brief The ConnectDialog class
 */
class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * \brief dump file formats
     */
    enum DumpFormat {
        Raw   = 1,
        Ascii = 2
    };

public:
    explicit ConnectDialog(QWidget *parent = 0);
    ~ConnectDialog();

    void accept();
    void closeEvent(QCloseEvent *event);

    QSettings Settings;

private:
    Ui::ConnectDialog *ui;

    /**
     * \brief fill connection settings combo boxes
     */
    void fillSettingsLists();

    /**
     * \brief preselect serial port connection configuration
     */
    void preselectPortConfig();
    QHash<QString, QString> getCfg();

    QHash<QString, QString> cfg;
    QStringList profileList;


signals:
    /**
     * \brief signal emitted 'open device' button has been clicked
     *
     * config parameter is a QHash with the following keys :
     *  - "device"
     *  - "baud_rate"
     *  - "data_bits"
     *  - "stop_bits"
     *  - "parity"
     *  - "flow_control"
     *  - "dump_enabled" dump enabled/disabled
     *  - "dump_file" full path of dump file
     *  - "dump_format" DumpFormat enum 'Raw' or 'Ascii'
     */
    void closeSettingsClicked(QString profile);
    void closedSignal();
private slots:

    void on_saveProfileButton_released();
    void on_removeProfileButton_released();
    void on_profileList_currentIndexChanged();
    void on_deviceList_highlighted(const QString &arg1);
};

#endif // CONNECTDIALOG_H
