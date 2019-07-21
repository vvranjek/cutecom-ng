#include "completerinput.h"
#include <QWidget>
#include <QAbstractItemModel>
#include <QStringListModel>
#include <QDebug>
#include <QFile>


CompleterInput::CompleterInput(QWidget *parent) :
    QLineEdit(parent)
    //edit(new QLineEdit(this))
{
    this->installEventFilter(this);
}

void CompleterInput::init()
{
    completer = new QCompleter(this);

    completer->setCompletionMode(QCompleter::InlineCompletion);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->setModel(modelFromFile(completerFile));
    completer->setWrapAround(true);
    completer->setCompletionRole(Qt::EditRole);
    this->setCompleter(completer);
    //this->setCompleter(NULL);
}


void CompleterInput::loadFromFile(const QString filename)
{
    completerFile = filename;
}

bool CompleterInput::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        /* Prevent tab key from loosing focus */
        if (keyEvent->key() == Qt::Key_Tab)
        {
            return true;
        }
    }
}

void CompleterInput::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {

    this->deselect();
    case Qt::Key_Up:
        //setCurrentIndex(0);
        //setItemText(0, history->previous());
        //fillList(lineEdit()->text());

        completer->setCurrentRow(completer->currentRow() + 1);
        completer->complete();
        break;

    case Qt::Key_Down:
        //setCurrentIndex(0);
        //setItemText(0, history->next());
        //fillList(lineEdit()->text());
        completer->setCurrentRow(completer->currentRow() - 1);
        completer->complete();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    {
        QString line = this->text().simplified();
        if (line.length() > 0)
        {
            // don't treat empty input

            completer->model()->insertRow(0);
            QModelIndex index = completer->model()->index(0,0);
            completer->model()->setData(index, line);
        }

        emit lineEntered(line);
        this->setText("");

        break;
    }
    case Qt::Key_Escape:
    {
        this->setText("");
    }
        break;

    case Qt::Key_Backspace:
    {
        QLineEdit::keyPressEvent(e);
        return;
    }
    case Qt::Key_Tab:
    {
        qDebug() << "Complete";
        completer->complete();
    }

    default:
        qDebug() << "Complete";
        completer->complete();

        QLineEdit::keyPressEvent(e);
        break;
    }
}

QAbstractItemModel *CompleterInput::modelFromFile(const QString& fileName)
{
    QString f = "/home/"+qgetenv("USER")+"/.cutecom/"+fileName+".history";
    QFile file(f);
    qDebug() << "Loading file: " << f;
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "Error opening file: " << f;
       //return new QStringListModel(completer);

        return NULL;
    }


    QStringList words;

    while (!file.atEnd()) {
        QByteArray line = file.readLine().simplified();





        if (!line.isEmpty())
            words << line.trimmed();

        qDebug() << "Loading hist: " << line.trimmed();
    }


    return new QStringListModel(words, completer);
}

