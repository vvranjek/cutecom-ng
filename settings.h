#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>

class settings : public QObject
{
    Q_OBJECT
public:
    explicit settings(QObject *parent = 0);

    static void saveSettings(QHash<QString, QString> hash, QString profile);
    static QHash<QString, QString> loadSettings(QString profile);
    static void addProfile(QString);
    static void setCurrentProfile(QString profile);
    static QString getCurrentProfile();
    static int getProfileCount();
    static QString getProfileName(int index);
    static void saveProfileList(QStringList profileList);
    static QStringList getProfileList();
    static QString getDeviceDescription(QString location);
    static QString getProfileNameFromLocation(QString location);
    static QString getDescriptionOfProfile(QString profile);
    static int getBaudRateOfProfile(QString profile);

private:

signals:

public slots:
};

#endif // SETTINGS_H
