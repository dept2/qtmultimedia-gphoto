#include "gphotoserviceplugin.h"
#include "gphotomediaservice.h"

#include <gphoto2/gphoto2-camera.h>

QMediaService* GPhotoServicePlugin::create(const QString &key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA))
        return new GPhotoMediaService;

    qWarning() << "GPhoto service plgin: unsupported key:" << key;
    return 0;
}

void GPhotoServicePlugin::release(QMediaService *service)
{
    delete service;
}

QByteArray GPhotoServicePlugin::defaultDevice(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (m_cameraDevices.isEmpty())
            updateDevices();

        return m_defaultCameraDevice;
    }

    return QByteArray();
}

QList<QByteArray> GPhotoServicePlugin::devices(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (m_cameraDevices.isEmpty())
            updateDevices();

        return m_cameraDevices;
    }

    return QList<QByteArray>();
}

QString GPhotoServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        if (m_cameraDevices.isEmpty())
            updateDevices();

        for (int i=0; i<m_cameraDevices.count(); i++)
            if (m_cameraDevices[i] == device)
                return m_cameraDescriptions[i];
    }

    return QString();
}

void GPhotoServicePlugin::updateDevices() const
{
    m_defaultCameraDevice.clear();
    m_cameraDevices.clear();
    m_cameraDescriptions.clear();

    GPContext *context;
    context = gp_context_new();
    Q_ASSERT(context != NULL);

    // List cameras
    CameraList *cameraList;
    GPPortInfoList *portInfoList;
    CameraAbilitiesList *abilitiesList;

    int ret = gp_list_new(&cameraList);
    if (ret < GP_OK)
        return;

    // Load all the port drivers we have...
    ret = gp_port_info_list_new(&portInfoList);
    if (ret < GP_OK)
        return;

    ret = gp_port_info_list_load(portInfoList);
    if (ret < 0)
        return;

//    ret = gp_port_info_list_count(portinfolist);
//    if (ret < 0)
//        return;

    // Load all the camera drivers we have...
    ret = gp_abilities_list_new(&abilitiesList);
    if (ret < GP_OK)
        return;

    ret = gp_abilities_list_load(abilitiesList, context);
    if (ret < GP_OK)
        return;

    // ... and autodetect the currently attached cameras.
    ret = gp_abilities_list_detect(abilitiesList, portInfoList, cameraList, context);
    if (ret < GP_OK)
        return;

    int cameraCount = gp_list_count(cameraList);

    for (int i = 0; i < cameraCount; ++i) {
      const char *name, *value;

      gp_list_get_name(cameraList, i, &name);
      gp_list_get_value(cameraList, i, &value);
      qDebug() << "Found" << name << "at port" << value;

//      CameraAbilities abilities;
//      gp_abilities_list_get_abilities(abilitiesList, i, &abilities);
//      qDebug() << abilities.operations;
//      qDebug() << "  capture image:" << ((abilities.operations | GP_OPERATION_CAPTURE_IMAGE) ? "true" : "false");
//      qDebug() << "  capture preview:" << ((abilities.operations | GP_OPERATION_CAPTURE_PREVIEW) ? "true" : "false");
//      qDebug() << "  capture video:" << ((abilities.operations | GP_OPERATION_CAPTURE_VIDEO) ? "true" : "false");
//      qDebug() << "  capture audio:" << ((abilities.operations | GP_OPERATION_CAPTURE_AUDIO) ? "true" : "false");

      m_cameraDevices.append(QByteArray(name));
      m_cameraDescriptions.append(QString::fromLatin1(value));
    }

    gp_abilities_list_free(abilitiesList);
    gp_port_info_list_free(portInfoList);
    gp_list_free(cameraList);

    if (!m_cameraDevices.isEmpty())
        m_defaultCameraDevice = m_cameraDevices.first();
}
