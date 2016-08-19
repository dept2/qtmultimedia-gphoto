#include "gphotoserviceplugin.h"
#include "gphotomediaservice.h"
#include "gphotofactory.h"

GPhotoServicePlugin::GPhotoServicePlugin()
  : m_factory(0)
{
}

GPhotoServicePlugin::~GPhotoServicePlugin()
{
  delete m_factory;
}

QMediaService* GPhotoServicePlugin::create(const QString &key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA))
        return new GPhotoMediaService(factory(), this);

    qWarning() << "GPhoto service plgin: unsupported key:" << key;
    return 0;
}

void GPhotoServicePlugin::release(QMediaService *service)
{
    delete service;
}

QByteArray GPhotoServicePlugin::defaultDevice(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA)
        return factory()->defaultCameraDevice();
    else
        return QByteArray();
}

QList<QByteArray> GPhotoServicePlugin::devices(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA)
        return factory()->cameraDevices();
    else
        return QList<QByteArray>();
}

QString GPhotoServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        const QList<QByteArray>& devices = factory()->cameraDevices();
        const int devicesCount = devices.count();
        for (int i = 0; i < devicesCount; ++i) {
            if (devices.at(i) == device)
                return factory()->cameraDescriptions().at(i);
        }
    }

    return QString();
}

GPhotoFactory* GPhotoServicePlugin::factory() const
{
    if (!m_factory)
        m_factory = new GPhotoFactory();

    return m_factory;
}
