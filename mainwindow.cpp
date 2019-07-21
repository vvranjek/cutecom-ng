/**
 * \file
 * <!--
 * Copyright 2015 Develer S.r.l. (http://www.develer.com/)
 * -->
 *
 * \brief main serial-ninja window
 *
 * \author Aurelien Rainone <aurelien@develer.com>
 */

#include <algorithm>
#include <iterator>

#include <QtUiTools/QUiLoader>
#include <QLineEdit>
#include <QPropertyAnimation>
#include <QShortcut>
#include <QFileDialog>
#include <QProgressDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSerialPortInfo>
#include <QTimer>
#include <QDebug>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectdialog.h"
#include "sessionmanager.h"
#include "outputmanager.h"
#include "searchhighlighter.h"
#include "settings.h"
#include "completerinput.h"

/// maximum count of document blocks for the bootom output
const int MAX_OUTPUT_LINES = 100;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    search_widget(0),
    search_input(0),
    progress_dialog(0),
    tempFileName("/home/"+qgetenv("USER")+"/.cutecom/tempfile"),
    tempFile(tempFileName)
{
    ui->setupUi(this);


    //qDebug() << GIT_VERSION;
    //QString version(GIT_VERSION);
    setWindowTitle("test");


#define _STR(X) #X
#define STR(X) _STR(X)
   // QString version = APP_REVISION;
//qDebug() << "MyApp revision " << version << endl;
//qDebug("VERSION: %s :\n", QString(VER1));



    // create session and output managers
    output_mgr = new OutputManager(this);
    session_mgr = new SessionManager(this);
    connect_dlg = new ConnectDialog(this);

    // show connection dialog
    connect(ui->settingsButton, &QAbstractButton::clicked, connect_dlg, &ConnectDialog::show);
    connect(connect_dlg, SIGNAL(closedSignal()), this, SLOT(onCloseDialog()));

    // handle reception of new data from serial port
    connect(session_mgr, &SessionManager::dataReceived, this, &MainWindow::handleDataReceived);

    // get data formatted for display and show it in output view
    connect(output_mgr, &OutputManager::dataConverted, this, &MainWindow::addDataToView);

    // get data formatted for display and show it in output view
    connect(ui->inputBox, &HistoryComboBox::lineEntered, this, &MainWindow::handleNewInput);

    //connect(ui->inputComplete, SIGNAL(CompleterInput::returnPressed), this, SLOT(MainWindow::handleNewInput));
    connect(ui->inputComplete, SIGNAL(CompleterInput::lineEntered()), this, SLOT(MainWindow::handleNewInput));

    // handle start/stop session
    connect(session_mgr, &SessionManager::sessionOpened, this, &MainWindow::handleSessionOpened);
    connect(session_mgr, &SessionManager::sessionClosed, this, &MainWindow::handleSessionClosed);

    // clear both output text when 'clear' is clicked
    connect(ui->clearButton, &QPushButton::clicked, ui->mainOutput, &QPlainTextEdit::clear);
    connect(ui->clearButton, &QPushButton::clicked, ui->bottomOutput, &QPlainTextEdit::clear);

    // connect open/close session slots
    connect(this, &MainWindow::openSession, session_mgr, &SessionManager::openSession);
    connect(connect_dlg, &ConnectDialog::closeSettingsClicked, this, &MainWindow::refreshSettings);
    connect(this, &MainWindow::closeSession, session_mgr, &SessionManager::closeSession);

    connect(ui->splitOutputBtn, &QPushButton::clicked, this, &MainWindow::toggleOutputSplitter);

    // load search widget and hide it
    QUiLoader loader;
    {
        QFile file(":/searchwidget.ui");
        file.open(QFile::ReadOnly);
        search_widget = loader.load(&file, ui->mainOutput);
        search_input = search_widget->findChild<QLineEdit*>("searchInput");
        search_prev_button = search_widget->findChild<QToolButton*>("previousButton");
        search_next_button = search_widget->findChild<QToolButton*>("nextButton");
    }
    search_widget->hide();

    // create the search results highlighter for main output
    SearchHighlighter *search_highlighter_main = new SearchHighlighter(ui->mainOutput->document());
    connect(search_input, &QLineEdit::textChanged, search_highlighter_main, &SearchHighlighter::setSearchString);

    // search result highlighter (without search cursor) for bottom output
    SearchHighlighter *search_highlighter_bottom = new SearchHighlighter(ui->bottomOutput->document(), false);
    connect(search_input, &QLineEdit::textChanged, search_highlighter_bottom, &SearchHighlighter::setSearchString);

    // connect search-related signals/slots
    connect(search_prev_button, &QPushButton::clicked,
            search_highlighter_main, &SearchHighlighter::previousOccurence);
    connect(search_next_button, &QPushButton::clicked,
            search_highlighter_main, &SearchHighlighter::nextOccurence);
    connect(search_highlighter_main, &SearchHighlighter::cursorPosChanged,
            this, &MainWindow::handleCursosPosChanged);
    connect(search_highlighter_main, &SearchHighlighter::totalOccurencesChanged,
            this, &MainWindow::handleTotalOccurencesChanged);
    connect(ui->searchButton, &QPushButton::toggled, this, &MainWindow::showSearchWidget);

    // additional configuration for bottom output
    ui->bottomOutput->hide();
    ui->bottomOutput->document()->setMaximumBlockCount(MAX_OUTPUT_LINES);

    // populate file transfer protocol combobox
    ui->protocolCombo->addItem("XModem", SessionManager::XMODEM);
    ui->protocolCombo->addItem("YModem", SessionManager::YMODEM);
    ui->protocolCombo->addItem("ZModem", SessionManager::ZMODEM);

    // transfer file over XModem protocol
    connect(ui->fileTransferButton, &QPushButton::clicked, this, &MainWindow::handleFileTransfer);
    connect(session_mgr, &SessionManager::fileTransferEnded, this, &MainWindow::handleFileTransferEnded);

    // fill end of line chars combobox
    ui->eolCombo->addItem(QStringLiteral("CR"), CR);
    ui->eolCombo->addItem(QStringLiteral("LF"), LF);
    ui->eolCombo->addItem(QStringLiteral("CR+LF"), CRLF);
    ui->eolCombo->addItem(QStringLiteral("None"), None);
    ui->eolCombo->setCurrentIndex(CR);
    _end_of_line = QByteArray("\n", 1);

    connect(ui->eolCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleEOLCharChanged);

    // install event filters
    ui->mainOutput->viewport()->installEventFilter(this);
    ui->bottomOutput->viewport()->installEventFilter(this);
    search_input->installEventFilter(this);
    installEventFilter(this);

    fillSettingsLists();

    ui->profileComboBox->addItems(settings::getProfileList());

    refreshSettings(settings::getCurrentProfile());

    // Set values of current profile
    QString _currentProfile;
    _currentProfile = settings::getCurrentProfile();
    ui->baudRateBox->setCurrentText(QString::number(settings::getBaudRateOfProfile(_currentProfile)));
    ui->profileComboBox->setCurrentText(_currentProfile);

    ui->inputBox->loadHistory(settings::getCurrentProfile());
    ui->inputComplete->loadFromFile(settings::getCurrentProfile());
    ui->inputComplete->init();
    ui->inputComplete->installEventFilter(this);

    ui->refreshButton->setIcon(QIcon("refresh"));
   //    ui->refreshButton->setIconSize(QSize(65,65));


    //QTimer *deviceTimer = new QTimer(this);
    //connect(deviceTimer, SIGNAL(timeout()), this, SLOT(updateDevices()));
    //deviceTimer->start(1000);

    ui->connectButton->toggle();
    ui->inputComplete->setFocus();
}

void MainWindow::updateDevices()
{
    if (ui->deviceComboBox->isActiveWindow()) {
        qDebug("Updating devices");

        QString currentText = ui->deviceComboBox->currentText();

        fillSettingsLists();

        ui->deviceComboBox->setCurrentText(currentText);

        refreshSettings(settings::getCurrentProfile());

    }
}

void MainWindow::fillSettingsLists()
{
    ui->deviceComboBox->clear();
    ui->baudRateBox->clear();
    ui->dataBitsComboBox->clear();
    ui->flowComboBox->clear();
    ui->parityComboBox->clear();
    // fill devices combo box
    QList<QSerialPortInfo> ports(QSerialPortInfo::availablePorts());
    for (int idx = 0; idx < ports.length(); ++idx)
    {
        const QSerialPortInfo& port_info = ports.at(idx);
        ui->deviceComboBox->addItem(port_info.systemLocation());

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

        ui->deviceComboBox->setItemData(idx, tooltip, Qt::ToolTipRole);
    }

    // Fill baudrate box
    QStringList baud_rates;
    baud_rates <<
                  QString::number(QSerialPort::Baud1200) << QString::number(QSerialPort::Baud2400);
    baud_rates <<
                  QString::number(QSerialPort::Baud4800) << QString::number(QSerialPort::Baud9600);
    baud_rates <<
                  QString::number(QSerialPort::Baud19200) << QString::number(QSerialPort::Baud38400);
    baud_rates <<
                  QString::number(QSerialPort::Baud57600) << QString::number(QSerialPort::Baud115200);
    ui->baudRateBox->addItems(baud_rates);

    // fill data bits combo box
    QStringList data_bits;
    data_bits <<
                 QString::number(QSerialPort::Data5) << QString::number(QSerialPort::Data6);
    data_bits <<
                 QString::number(QSerialPort::Data7) << QString::number(QSerialPort::Data8);
    ui->dataBitsComboBox->addItems(data_bits);
    ui->dataBitsComboBox->setCurrentText(QString::number(QSerialPort::Data8));

    // fill stop bits combo box
    ui->StopBitsComboBox->addItem(QStringLiteral("1"), QSerialPort::OneStop);
#ifdef Q_WS_WIN
    ui->StopBitsComboBox->addItem(QStringLiteral("1.5"), QSerialPort::OneAndHalfStop);
#endif
    ui->StopBitsComboBox->addItem(QStringLiteral("2"), QSerialPort::TwoStop);
    ui->StopBitsComboBox->setCurrentText("1");

    // fill parity combo box
    ui->parityComboBox->addItem(QStringLiteral("None"), QSerialPort::NoParity);
    ui->parityComboBox->addItem(QStringLiteral("Even"), QSerialPort::EvenParity);
    ui->parityComboBox->addItem(QStringLiteral("Odd"), QSerialPort::OddParity);
    ui->parityComboBox->addItem(QStringLiteral("Space"), QSerialPort::SpaceParity);
    ui->parityComboBox->addItem(QStringLiteral("Mark"), QSerialPort::MarkParity);
    ui->parityComboBox->setCurrentText("None");

    // fill flow control
    ui->flowComboBox->addItem(QStringLiteral("No flow"), QSerialPort::NoFlowControl);
    ui->flowComboBox->addItem(QStringLiteral("Hardware"), QSerialPort::HardwareControl);
    ui->flowComboBox->addItem(QStringLiteral("Software"), QSerialPort::SoftwareControl);
    ui->flowComboBox->setCurrentText("No flow");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onCloseDialog()
{
    qDebug() << "Dialog closed";
    ui->profileComboBox->clear();
    ui->profileComboBox->addItems(settings::getProfileList());
    refreshSettings(settings::getCurrentProfile());
}

void MainWindow::refreshSettings(QString profile)
{
        QHash<QString, QString> cfg;
        cfg = settings::loadSettings(profile);
        ui->profileComboBox->setCurrentText(profile);
        ui->hardwareLabel->setText(cfg[QStringLiteral("description")]);
	
        // Find and set existing port index
        QString device = cfg[QStringLiteral("device")];

        // Set index only if available in settings, otherwise leave is it was
        int index = ui->deviceComboBox->currentIndex();
        if (ui->deviceComboBox->findText(device) >= 0) {
            index = ui->deviceComboBox->findText(device);
        }

        qDebug("Setting index: %d", index);
        ui->deviceComboBox->setCurrentIndex(index);

        ui->baudRateBox->setCurrentText(cfg[QStringLiteral("baud_rate")]);
        ui->parityComboBox->setCurrentText(cfg[QStringLiteral("parity")]);
        ui->flowComboBox->setCurrentText(cfg[QStringLiteral("flow_control")]);
        ui->dataBitsComboBox->setCurrentText(cfg[QStringLiteral("data_bits")]);
        ui->StopBitsComboBox->setCurrentText(cfg[QStringLiteral("stop_bits")]);
}

void MainWindow::handleSessionOpened()
{
    // clear output buffer
    output_mgr->clear();

    // clear both output windows
    ui->mainOutput->clear();
    ui->bottomOutput->clear();

    ui->connectButton->setChecked(true);
    ui->connectButton->setText("Disconnect");

    // enable file transfer and input line
    ui->fileTransferButton->setEnabled(true);
    ui->inputBox->setEnabled(true);
    ui->inputBox->setFocus();
}

void MainWindow::handleSessionClosed()
{
    ui->connectButton->setChecked(false);
    ui->connectButton->setText("Connect");

    // disable file transfer and input line
    ui->fileTransferButton->setDisabled(true);
    ui->inputBox->setDisabled(true);
}

void MainWindow::handleFileTransfer()
{
    QString filename = QFileDialog::getOpenFileName(
                this, QStringLiteral("Select file to send transfer"));

    if (filename.isNull())
        return;

    Q_ASSERT_X(progress_dialog == 0, "MainWindow::handleFileTransfer()", "progress_dialog should be null");

    // display a progress dialog
    progress_dialog = new QProgressDialog(this);
    connect(progress_dialog, &QProgressDialog::canceled,
            session_mgr, &SessionManager::handleTransferCancelledByUser);

    progress_dialog->setRange(0, 100);
    progress_dialog->setWindowModality(Qt::ApplicationModal);
    progress_dialog->setLabelText(
                QStringLiteral("Initiating connection with receiver"));

    // update progress dialog
    connect(session_mgr, &SessionManager::fileTransferProgressed,
            this, &MainWindow::handleFileTransferProgressed);

    int protocol = ui->protocolCombo->currentData().toInt();
    session_mgr->transferFile(filename, static_cast<SessionManager::Protocol>(protocol));

    // disable UI elements acting on QSerialPort instance, as long as
    // objectds involved in FileTransferred are not destroyed or back
    // to their pre-file-transfer state
    ui->fileTransferButton->setEnabled(false);
    ui->inputBox->setEnabled(false);

    // progress dialog event loop
    progress_dialog->exec();

    delete progress_dialog;
    progress_dialog = 0;
}

void MainWindow::handleFileTransferProgressed(int percent)
{
    if (progress_dialog)
    {
        progress_dialog->setValue(percent);
        progress_dialog->setLabelText(QStringLiteral("Transferring file"));
    }
}

void MainWindow::handleFileTransferEnded(FileTransfer::TransferError error)
{
    switch (error)
    {
    case FileTransfer::LocalCancelledError:
        break;
    case FileTransfer::NoError:
        QMessageBox::information(this, tr("Cutecom-ng"), QStringLiteral("File transferred successfully"));
        break;
    default:
        progress_dialog->setLabelText(FileTransfer::errorString(error));
        break;
    }

    // re-enable UI elements acting on QSerialPort instance
    ui->fileTransferButton->setEnabled(true);
    ui->inputBox->setEnabled(true);
}

void MainWindow::handleNewInput(QString entry)
{
    // if session is not open, this also keeps user input
    if (session_mgr->isSessionOpen())
    {
        QByteArray to_send(entry.toLocal8Bit());
        to_send.append(_end_of_line);
        session_mgr->sendToSerial(to_send);
        ui->inputBox->clearEditText();
    }
}

void MainWindow::addDataToView(const QByteArray data)
{
    QString newdata;
    const QString textdata = QString::fromLocal8Bit(data.data());

    if (!ui->asciiHexButton->isChecked()) {

        // problem : QTextEdit interprets a '\r' as a new line, so if a buffer ending
        //           with '\r\n' happens to be cut in the middle, there will be 1 extra
        //           line jump in the QTextEdit. To prevent we remove ending '\r' and
        //           prepend them to the next received buffer

        // flag indicating that the previously received buffer ended with CR
        static bool prev_ends_with_CR = false;

        if (prev_ends_with_CR)
        {
            // CR was removed at the previous buffer, so now we prepend it
            newdata.append('\r');
            prev_ends_with_CR = false;
        }

        if (textdata.length() > 0)
        {
            QString::const_iterator end_cit = textdata.cend();
            if (textdata.endsWith('\r'))
            {
                // if buffer ends with CR, we don't copy it
                end_cit--;
                prev_ends_with_CR = true;
            }
            std::copy(textdata.begin(), end_cit, std::back_inserter(newdata));
        }
    }
    else {

        newdata.clear();

        // Create a hex value for each char
        for (int i = 0; i < data.size(); i++) {
            QString number = QString("%1").arg((unsigned char)data.at(i), 2, 16, QChar('0'));

            newdata.append(number.toUpper());
            newdata.append(" ");
        }
    }

    // record end cursor position before adding text
    QTextCursor prev_end_cursor(ui->mainOutput->document());
    prev_end_cursor.movePosition(QTextCursor::End);

    if (ui->bottomOutput->isVisible())
    {
        // append text to the top output and stay at current position
        QTextCursor cursor(ui->mainOutput->document());
        cursor.insertText(newdata);
        cursor.movePosition(QTextCursor::End);
    }
    else
    {
        // append text to the top output and scroll down
        ui->mainOutput->insertPlainText(newdata);
        ui->mainOutput->moveCursor(QTextCursor::End);
    }
    ui->mainOutput->moveCursor(QTextCursor::End);

    // append text to bottom output and scroll
    ui->bottomOutput->moveCursor(QTextCursor::End);
    ui->bottomOutput->insertPlainText(newdata);
    //ui->bottomOutput->insertPlainText(textdata);
    appendToLogFile(newdata);
}

void MainWindow::handleDataReceived(const QByteArray &data)
{
    (*output_mgr) << data;
}

void MainWindow::toggleOutputSplitter()
{
    ui->bottomOutput->setVisible(!ui->bottomOutput->isVisible());
}

bool MainWindow::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::Resize && target == ui->mainOutput->viewport())
    {
        // re position search widget when main output inner size changes
        // this takes into account existence of vertical scrollbar
        QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);
        search_widget->move(resizeEvent->size().width() - search_widget->width(), 0);
    }
    else if (event->type() == QEvent::Wheel)
    {
        // mouse wheel allowed only for main output in 'browsing mode'
        if (target == ui->bottomOutput->viewport() ||
                (target == ui->mainOutput->viewport() && !ui->bottomOutput->isVisible()))
            return true;
    }
    else if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (ui->searchButton->isChecked())
        {
            // hide search widget on Escape key press
            if (keyEvent->key() == Qt::Key_Escape)
                ui->searchButton->toggle();
        }
        else
        {
            // show search widget on Ctrl-F
            if (keyEvent->key() == Qt::Key_F && keyEvent->modifiers() == Qt::ControlModifier)
                ui->searchButton->toggle();
        }
        if (target == search_input)
        {
            if (keyEvent->key() == Qt::Key_Return)
            {
                if (keyEvent->modifiers() == Qt::ShiftModifier)
                    search_prev_button->click();
                else
                    search_next_button->click();
            }
        }
    }

    // base class behaviour
    return QMainWindow::eventFilter(target, event);
}

void MainWindow::showSearchWidget(bool show)
{
    // record which widget had focus before showing search widget, in order
    // to return focus to it when search widget is hidden
    static QWidget *prevFocus = 0;

    QPropertyAnimation *animation = new QPropertyAnimation(search_widget, "pos");
    animation->setDuration(150);

    // place search widget at the rightmost position of mainoutput viewport
    QPoint showed_pos(ui->mainOutput->viewport()->width() - search_widget->width(), 0);
    QPoint hidden_pos(ui->mainOutput->viewport()->width() - search_widget->width(), -search_widget->height());
    animation->setStartValue(show ? hidden_pos : showed_pos);
    animation->setEndValue(show ? showed_pos : hidden_pos);

    if (show)
    {
        prevFocus = QApplication::focusWidget();
        search_widget->setVisible(show);
        search_input->setFocus();
    }
    else
    {
        connect(animation, &QPropertyAnimation::destroyed, search_widget, &QWidget::hide);
        prevFocus->setFocus();
    }
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::handleCursosPosChanged(int pos)
{
    // move cursor
    QTextCursor text_cursor = ui->mainOutput->textCursor();
    text_cursor.setPosition(pos);

    // ensure search result cursor is visible
    ui->mainOutput->ensureCursorVisible();
    ui->mainOutput->setTextCursor(text_cursor);
}

void MainWindow::handleTotalOccurencesChanged(int total_occurences)
{
    if (total_occurences == 0)
        search_input->setStyleSheet("QLineEdit{background-color: red;}");
    else
        search_input->setStyleSheet("");
}

void MainWindow::handleEOLCharChanged(int index)
{
    Q_UNUSED(index)

    switch(ui->eolCombo->currentData().toInt())
    {
    case CR:
        _end_of_line = QByteArray("\r", 1);
        break;
    case LF:
        _end_of_line = QByteArray("\n", 1);
        break;
    case CRLF:
        _end_of_line = QByteArray("\r\n", 2);
        break;
    case None:
        _end_of_line.clear();
        break;
    default:
        Q_ASSERT_X(false, "MainWindow::handleEOLCharChanged",
                   "unknown EOL char value: " + ui->eolCombo->currentData().toInt());
        break;
    }
}

void MainWindow::on_connectButton_released()
{
    settings::setCurrentProfile(ui->profileComboBox->currentText());
    if (ui->connectButton->isChecked()) {
        tempFile.remove();
    }

    if (ui->connectButton->isChecked()) {
        QHash<QString, QString> current_settings;

        current_settings[QStringLiteral("device")] = ui->deviceComboBox->currentText();
        current_settings[QStringLiteral("description")] = ui->hardwareLabel->text();
        current_settings[QStringLiteral("baud_rate")] = ui->baudRateBox->currentText();
        current_settings[QStringLiteral("data_bits")] = ui->dataBitsComboBox->currentText();
        current_settings[QStringLiteral("stop_bits")] = ui->StopBitsComboBox->currentText();
        current_settings[QStringLiteral("parity")] = ui->parityComboBox->currentData().toString();
        current_settings[QStringLiteral("flow_control")] = ui->flowComboBox->currentData().toString();

        settings::saveSettings(current_settings, "Current_settings");
        emit openSession(QString::fromLocal8Bit("Current_settings"));
    }
    else {
        emit closeSession();
    }
}

void MainWindow::on_profileComboBox_currentTextChanged(const QString &arg1)
{
    refreshSettings(arg1);
}

void MainWindow::on_deviceComboBox_highlighted(const QString &arg1)
{
    ui->hardwareLabel->setText(settings::getDescriptionOfProfile(arg1));
}

void MainWindow::on_deviceComboBox_activated(const QString &arg1)
{

}

void MainWindow::on_profileComboBox_highlighted(const QString &arg1)
{
    ui->baudRateBox->setCurrentText(QString::number(settings::getBaudRateOfProfile(arg1)));

    // Find correct device if it exists
    QString _description = settings::getDescriptionOfProfile(arg1);

    for (int i = 0; i < ui->deviceComboBox->count(); i++) {
        //qDebug() << "Compare: "<< _description << " : " << settings::getDeviceDescription(ui->deviceComboBox->itemText(i));

        if (QString::compare(_description, settings::getDeviceDescription(ui->deviceComboBox->itemText(i))) == 0) {
            qDebug() << "Comparicon found";
            ui->deviceComboBox->setCurrentIndex(i);
            ui->deviceComboBox->setCurrentText(ui->deviceComboBox->currentText());
            return;
        }
        else {
            ui->deviceComboBox->setCurrentText("No device found");
        }
    }
}

void MainWindow::appendToLogFile(QString text)
{

    // Create path if it doesnt exist
    if (!tempFile.exists()) {
        QDir _dir;
        _dir.mkdir("/home/"+qgetenv("USER")+"/.cutecom/");
        qDebug() << "Tempfile doesn't exist";
    }

    if (tempFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << "Appending to log file:" << text;

        QTextStream stream(&tempFile);
        stream << text;
    }
    else {
        qDebug() << "Error opening temp file";
    }
    tempFile.close();
}

void MainWindow::on_clearButton_clicked()
{
    tempFile.remove();
}

void MainWindow::on_saveButton_released()
{
    while (true) {
        QString fileName = QFileDialog::getOpenFileName(this, QStringLiteral("Save session to.."));

        if (fileName.isNull()) {
            qDebug() << "Filename error";
            break;
        }
        else {
            QFile file(fileName);

            qDebug() << "Copying: " << tempFileName << " to " << fileName;

            // Check if file exists and ask if overwrite
            if (QFile::exists(fileName))
            {
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(this, "Overwrite file?", "File exists, overwrite?", QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
                if (reply == QMessageBox::Yes) {
                    qDebug() << "Yes, overwite was clicked";
                    QFile::remove(fileName);
                    //break;
                }
                else if (reply == QMessageBox::Cancel) {
                    qDebug() << "Yes was *not* clicked";
                    return;
                }
                else if (reply == QMessageBox::No) {
                    qDebug() << "Yes was *not* clicked";
                    continue;
                }
                else {
                    return;
                }
            }
            else {
                if (!file.open(QIODevice::WriteOnly)) {
                    QMessageBox::information(this, tr("Unable to open file"),
                                             file.errorString());
                    return;
                }
            }

            // This is a temp solution because File.copy doesn't work for some reason
            QString cpString = "cp "+tempFileName+" "+fileName;
            //  int cp_error = system(cpString.toLatin1());

            if (system(cpString.toLatin1()) > 0) {
                QMessageBox::warning(this, "Warning", "Error " + cpString);
                       // QMessageBox::warning(this, "Warning", "Error ("+QStrin)
            }
            else {
                break;
            }

            //QFile::copy(tempFileName, fileName);
        }
    }
}




void MainWindow::on_refreshButton_released()
{
    updateDevices();
}
