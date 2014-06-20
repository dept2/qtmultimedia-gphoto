#ifndef GPHOTOSERVICEPLUGIN_H
#define GPHOTOSERVICEPLUGIN_H

#include <qmediaserviceproviderplugin.h>


class GPhotoServicePlugin
    : public QMediaServiceProviderPlugin
    , public QMediaServiceSupportedDevicesInterface
    , public QMediaServiceDefaultDeviceInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaServiceSupportedDevicesInterface)
    Q_INTERFACES(QMediaServiceDefaultDeviceInterface)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0" FILE "gphoto.json")
public:
    QMediaService* create(const QString &key);
    void release(QMediaService *service);

    QByteArray defaultDevice(const QByteArray &service) const;
    QList<QByteArray> devices(const QByteArray &service) const;
    QString deviceDescription(const QByteArray &service, const QByteArray &device);

private:
    void updateDevices() const;

    mutable QByteArray m_defaultCameraDevice;
    mutable QList<QByteArray> m_cameraDevices;
    mutable QStringList m_cameraDescriptions;
};

#endif // GPHOTOSERVICEPLUGIN_H
