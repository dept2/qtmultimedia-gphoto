#include "gphotoserviceplugin.h"
#include "gphotomediaservice.h"
#include "gphotofactory.h"

GPhotoServicePlugin::GPhotoServicePlugin()
{}

GPhotoServicePlugin::~GPhotoServicePlugin()
{}

QMediaService* GPhotoServicePlugin::create(const QString &key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA))
        return new GPhotoMediaService(factory());

    qWarning() << "GPhoto service plugin: unsupported key:" << key;
    return nullptr;
}

void GPhotoServicePlugin::release(QMediaService *service)
{
    delete service;
}

QByteArray GPhotoServicePlugin::defaultDevice(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA)
        return factory()->defaultCameraName();
    else
        return QByteArray();
}

QList<QByteArray> GPhotoServicePlugin::devices(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA)
        return factory()->cameraNames();
    else
        return QList<QByteArray>();
}

QString GPhotoServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        return device;
    }

    return QString();
}

GPhotoFactory* GPhotoServicePlugin::factory() const
{
    if (!m_factory) {
        auto factory = std::unique_ptr<GPhotoFactory>(new GPhotoFactory);
        if (!factory->init()) {
            qWarning() << "GPhoto service plugin: unable to initialize GPhotoFactory";
            return nullptr;
        }
        m_factory = std::move(factory);
    }

    return m_factory.get();
}
