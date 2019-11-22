#ifndef GPHOTOSERVICEPLUGIN_H
#define GPHOTOSERVICEPLUGIN_H

#include <memory>

#include <qmediaserviceproviderplugin.h>

class GPhotoFactory;

class GPhotoServicePlugin final
    : public QMediaServiceProviderPlugin
    , public QMediaServiceSupportedDevicesInterface
    , public QMediaServiceDefaultDeviceInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaServiceSupportedDevicesInterface)
    Q_INTERFACES(QMediaServiceDefaultDeviceInterface)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.mediaserviceproviderfactory/5.0" FILE "gphoto.json")
public:
    GPhotoServicePlugin();
    ~GPhotoServicePlugin();

    GPhotoServicePlugin(GPhotoServicePlugin&&) = delete;
    GPhotoServicePlugin& operator=(GPhotoServicePlugin&&) = delete;

    QMediaService* create(const QString &key) override;
    void release(QMediaService *service) override;

    QByteArray defaultDevice(const QByteArray &service) const override;
    QList<QByteArray> devices(const QByteArray &service) const override;
    QString deviceDescription(const QByteArray &service, const QByteArray &device) override;

private:
    Q_DISABLE_COPY(GPhotoServicePlugin)

    GPhotoFactory* factory() const;

    mutable std::unique_ptr<GPhotoFactory> m_factory;
};

#endif // GPHOTOSERVICEPLUGIN_H
