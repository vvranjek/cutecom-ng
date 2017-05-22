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

#include "connectdialog.h"
#include "ui_connectdialog.h"

#include <QDebug>
#include <QList>
#include <QHash>
#include <QtSerialPort>
#include <QSerialPortInfo>
#include "settings.h"

ConnectDialog::ConnectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectDialog)
{
    ui->setupUi(this);

    // fill the combo box values
    fillSettingsLists();

    // for device take 1st device listed (if any)
    QString default_device;
    if (ui->deviceList->count() > 0)
        default_device = ui->deviceList->itemText(0);

    // define a default configuration
    QHash<QString, QString> default_cfg;
    default_cfg[QStringLiteral("device")] = default_device;
    default_cfg[QStringLiteral("baud_rate")] = QString::number(QSerialPort::Baud115200);
    default_cfg[QStringLiteral("data_bits")] = QString::number(QSerialPort::Data8);
    default_cfg[QStringLiteral("stop_bits")] = QStringLiteral("1");
    default_cfg[QStringLiteral("parity")] = QStringLiteral("None");
    default_cfg[QStringLiteral("flow_control")] = QStringLiteral("None");

    // define the default values for output dump
    default_cfg[QStringLiteral("dump_enabled")] = QString::number(0);
    default_cfg[QStringLiteral("dump_file")] = QStringLiteral("serial-ninja.dump");
    default_cfg[QStringLiteral("dump_format")] = QString::number(Raw);

    profileList = settings::getProfileList();
    ui->profileList->addItems(profileList);
    preselectPortConfig();
}

ConnectDialog::~ConnectDialog()
{
    delete ui;
}

void ConnectDialog::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    emit closedSignal();
}

void ConnectDialog::fillSettingsLists()
{
    // fill devices combo box
    QList<QSerialPortInfo> ports(QSerialPortInfo::availablePorts());
    for (int idx = 0; idx < ports.length(); ++idx)
    {
        const QSerialPortInfo& port_info = ports.at(idx);
        ui->deviceList->addItem(port_info.systemLocation());

        // construct description tooltip
        QString tooltip;

        // add description if not empty
        if (!port_info.description().isEmpty())
            tooltip.append(port_info.description());
        if (!port_info.manufacturer().isEmpty())
        {
            // add ' / manufacturer' if not empty
            if (!tooltip.isEmpty())
                tooltip.push_back(QStringLiteral(" / "));
            tooltip.append(port_info.manufacturer());
        }
        // assign portName
        if (tooltip.isEmpty())
            tooltip = port_info.portName();

        ui->deviceList->setItemData(idx, tooltip, Qt::ToolTipRole);
    }

    // fill baud rates combo box
    QStringList baud_rates;
    baud_rates <<
                  QString::number(QSerialPort::Baud1200) << QString::number(QSerialPort::Baud2400);
    baud_rates <<
                  QString::number(QSerialPort::Baud4800) << QString::number(QSerialPort::Baud9600);
    baud_rates <<
                  QString::number(QSerialPort::Baud19200) << QString::number(QSerialPort::Baud38400);
    baud_rates <<
                  QString::number(QSerialPort::Baud57600) << QString::number(QSerialPort::Baud115200);
    ui->baudRateList->addItems(baud_rates);

    // fill data bits combo box
    QStringList data_bits;
    data_bits <<
                 QString::number(QSerialPort::Data5) << QString::number(QSerialPort::Data6);
    data_bits <<
                 QString::number(QSerialPort::Data7) << QString::number(QSerialPort::Data8);
    ui->dataBitsList->addItems(data_bits);

    // fill stop bits combo box
    ui->stopBitsList->addItem(QStringLiteral("1"), QSerialPort::OneStop);
#ifdef Q_WS_WIN
    ui->stopBitsList->addItem(QStringLiteral("1.5"), QSerialPort::OneAndHalfStop);
#endif
    ui->stopBitsList->addItem(QStringLiteral("2"), QSerialPort::TwoStop);

    // fill parity combo box
    ui->parityList->addItem(QStringLiteral("None"), QSerialPort::NoParity);
    ui->parityList->addItem(QStringLiteral("Even"), QSerialPort::EvenParity);
    ui->parityList->addItem(QStringLiteral("Odd"), QSerialPort::OddParity);
    ui->parityList->addItem(QStringLiteral("Space"), QSerialPort::SpaceParity);
    ui->parityList->addItem(QStringLiteral("Mark"), QSerialPort::MarkParity);

    // fill flow control
    ui->flowControlList->addItem(QStringLiteral("None"), QSerialPort::NoFlowControl);
    ui->flowControlList->addItem(QStringLiteral("Hardware"), QSerialPort::HardwareControl);
    ui->flowControlList->addItem(QStringLiteral("Software"), QSerialPort::SoftwareControl);
}

void ConnectDialog::preselectPortConfig()
{
    QHash<QString, QString> cfg;
    qDebug() << "Port config";

    cfg = settings::loadSettings(ui->profileList->currentText());

    ui->deviceList->setCurrentText(cfg[QStringLiteral("device")]);
    ui->baudRateList->setCurrentText(cfg[QStringLiteral("baud_rate")]);
    ui->dataBitsList->setCurrentText(cfg[QStringLiteral("data_bits")]);
    ui->stopBitsList->setCurrentText(cfg[QStringLiteral("stop_bits")]);
    ui->portInfoLabel->setText(cfg[QStringLiteral("description")]);

    ui->parityList->setCurrentText(cfg[QStringLiteral("parity")]);
    ui->flowControlList->setCurrentText(cfg[QStringLiteral("flow_control")]);

    ui->dumpFile->setChecked(cfg[QStringLiteral("dump_enabled")] == "1");
    ui->dumpPath->setText(cfg[QStringLiteral("dump_file")]);
    ui->dumpRawFmt->setChecked(cfg["dump_format"] == QString::number(Raw));
    ui->dumpTextFmt->setChecked(cfg["dump_format"] == QString::number(Ascii));
}

void ConnectDialog::accept()
{
    // create a serial port config object from current selection
    cfg = getCfg();
    hide();

    settings::saveSettings(cfg, ui->profileList->currentText());
    settings::setCurrentProfile(ui->profileList->currentText());

    emit closeSettingsClicked(ui->profileList->currentText());
}

QHash<QString, QString> ConnectDialog::getCfg()
{
    cfg[QStringLiteral("device")] = ui->deviceList->currentText();
    cfg[QStringLiteral("description")] = settings::getDeviceDescription(ui->deviceList->currentText());
    cfg[QStringLiteral("baud_rate")] = ui->baudRateList->currentText();
    cfg[QStringLiteral("data_bits")] = ui->dataBitsList->currentText();
    cfg[QStringLiteral("stop_bits")] = ui->stopBitsList->itemData(
                ui->stopBitsList->currentIndex()).toString();
    cfg[QStringLiteral("parity")] = ui->parityList->itemData(
                ui->parityList->currentIndex()).toString();
    cfg[QStringLiteral("flow_control")] = ui->flowControlList->itemData(
                ui->flowControlList->currentIndex()).toString();
    cfg[QStringLiteral("dump_enabled")] = ui->dumpFile->isChecked() ? "1" : "0";
    cfg[QStringLiteral("dump_file")] = ui->dumpPath->text();
    cfg[QStringLiteral("dump_format")] = QString::number(ui->dumpRawFmt->isChecked() ? Raw : Ascii);

    return cfg;
}

void ConnectDialog::on_saveProfileButton_released()
{
    int index = profileList.indexOf(ui->profileList->currentText());
    if (index >= 0) {
        qDebug() << "Index: " << index;
        profileList.replace(index, ui->profileList->currentText());
    }
    else {
        profileList.append(ui->profileList->currentText());
        ui->profileList->addItem(ui->profileList->currentText());
    }

    cfg = getCfg();

    settings::saveProfileList(profileList);
    settings::saveSettings(cfg, ui->profileList->currentText());
}

void ConnectDialog::on_removeProfileButton_released()
{
    int index = profileList.indexOf(ui->profileList->currentText());
    qDebug() << "Before remove list: " << profileList;
    profileList.removeAt(index);
    qDebug() << "After remove list: " << profileList;
    ui->profileList->clear();
    ui->profileList->addItems(profileList);
    settings::saveProfileList(profileList);

}

void ConnectDialog::on_profileList_currentIndexChanged()
{
    cfg = settings::loadSettings(ui->profileList->currentText());
    preselectPortConfig();
}

void ConnectDialog::on_deviceList_highlighted(const QString &arg1)
{
    QString _description = settings::getDeviceDescription(arg1);

    if (_description != "none") {
        ui->portInfoLabel->setText(_description);
    }
    else {
        ui->portInfoLabel->setText("Error: Not connected");
    }
}
