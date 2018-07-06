#include "settings.h"
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>

#define PROFILES "Profiles"
#define ALL_PROFILES "All"

settings::settings(QObject *parent) : QObject(parent)
{

}

void settings::saveSettings(QHash<QString, QString> hash, QString profile)
{
    QSettings Settings;
    Settings.beginGroup(profile);

    QHash<QString, QString>::const_iterator i = hash.constBegin();
    while (i != hash.constEnd()) {
        Settings.setValue(i.key(), i.value());
        qDebug() << "Save: " << i.key() << " - " << i.value();
        ++i;
    }
    Settings.endGroup();
}

void settings::saveProfileList(QStringList profileList)
{
    QSettings settings;
    settings.beginGroup(PROFILES);
    settings.beginWriteArray(ALL_PROFILES);
    qDebug() << "Profile list size: " << profileList.size();
    for (int i = 0; i < profileList.size(); ++i) {
        qDebug() << "Save profile list: " << i << " - " <<  profileList.at(i);
        settings.setArrayIndex(i);
        settings.setValue("name", profileList.at(i));
        qDebug() << "Done: " << i;
    }
    settings.endArray();
    settings.endGroup();

}

QStringList settings::getProfileList()
{
    QSettings settings;
    QStringList profileList;
    settings.beginGroup(PROFILES);
    int size = settings.beginReadArray(ALL_PROFILES);
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        profileList.append(settings.value("name").toString());
        //qDebug() << "Get profile list: " << i << " - " << settings.value("name").toString();

    }
    //qDebug() << "Profile list: " << profileList;
    settings.endArray();
    settings.endGroup();
    return profileList;
}

void settings::addProfile(QString profile)
{
    QSettings settings;

    settings.beginGroup (PROFILES);
    int size = settings.beginReadArray(ALL_PROFILES);
    qDebug() << "Adding profile size: " << size;
    settings.setArrayIndex (size);
    settings.setValue("name", profile);
    settings.endArray();
    settings.endGroup();

}

QString settings::getProfileNameFromLocation(QString location)
{

    QStringList profileList;
    QHash<QString, QString> _hash;

    profileList = getProfileList();

    qDebug() << profileList;

    for (int i = 0; i < profileList.size(); i++) {

        _hash = loadSettings(profileList.at(i));

        qDebug() << "Device: " << _hash[QStringLiteral("device")] << " - " << profileList.at(i);


        if (_hash[QStringLiteral("device")] == location) {
            qDebug() << "FAOUND profile: " << profileList.at(i);
            return profileList.at(i);
        }


    }
    //qDebug() << "Profile list: " << profileList;

    return "not found";

}

void settings::setCurrentProfile(QString profile)
{
    QSettings settings;
    settings.beginGroup ("CurrentProfile");
    settings.setValue("CurrentProfile", profile);
    settings.endGroup();
}

int settings::getProfileCount()
{
    QSettings settings;
    settings.beginGroup(PROFILES);
    int size = settings.beginReadArray(ALL_PROFILES);
    qDebug() << "Profile count: " << size;
    settings.endArray();
    return size;
}

QString settings::getCurrentProfile()
{
    QSettings settings;
    settings.beginGroup ("CurrentProfile");
    QString currentProfile = settings.value("CurrentProfile").toString();
    settings.endGroup();
    return currentProfile;
}

QHash<QString, QString> settings::loadSettings(QString profile)
{
    QSettings Settings;
    QHash<QString, QString> hash;
    Settings.beginGroup(profile);
    QStringList keys = Settings.childKeys();
    foreach (QString key, keys) {
        hash[key] = Settings.value(key).toString(); //Settings.value(key).toInt();
        //qDebug() << "Load: " << key << " - " << Settings.value(key);
    }
    Settings.endGroup();

    return hash;
}

QString settings::getDeviceDescription(QString location)
{
    QList<QSerialPortInfo> ports(QSerialPortInfo::availablePorts());
    for (int idx = 0; idx < ports.length(); ++idx)
    {
        const QSerialPortInfo& port_info = ports.at(idx);

        //if (location.compare(port_info.systemLocation()) == 0){
        if (port_info.systemLocation() == location) {

            // construct description description
            QString description;

            // add description if not empty
            if (!port_info.description().isEmpty()) {
                description.append(port_info.description());
            }
            if (!port_info.manufacturer().isEmpty())
            {
                // add ' / manufacturer' if not empty
                if (!description.isEmpty())
                    description.push_back(QStringLiteral(" / "));
                description.append(port_info.manufacturer());
            }

//            // Add serial number
//            if (!port_info.serialNumber().isEmpty()) {
//                if (!description.isEmpty()) {
//                    description.push_back(QStringLiteral(" / "));
//                }
//                description.append(port_info.serialNumber());
//            }

            return description;
        }
    }

    return "none";
}

QString settings::getDescriptionOfProfile(QString profile)
{
    QHash<QString, QString> _hash;
    _hash = loadSettings(profile);
    qDebug() << "Profile " << profile << "description: " << _hash[QStringLiteral("description")];
    return _hash[QStringLiteral("description")];
}

int settings::getBaudRateOfProfile(QString profile)
{
    QHash<QString, QString> _hash;
    _hash = loadSettings(profile);
    return _hash[QStringLiteral("baud_rate")].toInt();
}



