#ifndef COMPLETERINPUT_H
#define COMPLETERINPUT_H

#include <QLineEdit>
#include <QCompleter>
#include <QKeyEvent>
#include <QObject>


class CompleterInput  : public QLineEdit/*, public QObject*/
{

private:
    QCompleter *completer;
    QString completerFile;

    QAbstractItemModel *modelFromFile(const QString& fileName);

public:
    CompleterInput(QWidget *parent = 0);

    void init();
    void loadFromFile(const QString filename);

protected:
    virtual void keyPressEvent(QKeyEvent *e);

signals:
    //void lineEntered(const QString);
};

#endif // COMPLETERINPUT_H
