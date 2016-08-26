#include "gphotofactory.h"

#include <QDebug>

GPhotoFactory::GPhotoFactory()
    : m_context(gp_context_new())
    , m_cameraAbilitiesList(0)
    , m_portInfoList(0)
{
    if (!m_context) {
        qWarning() << "Unable to create GPhoto context";
        return;
    }

    initCameraAbilitiesList();
    initPortInfoList();
}

GPhotoFactory::~GPhotoFactory()
{
    gp_port_info_list_free(m_portInfoList);
    gp_abilities_list_free(m_cameraAbilitiesList);
    gp_context_unref(m_context);
}

QMap<QByteArray, QByteArray> GPhotoFactory::cameraDevices() const
{
    updateDevices();
    return m_cameraDevices;
}

QStringList GPhotoFactory::cameraDescriptions() const
{
    updateDevices();
    return m_cameraDescriptions;
}

QByteArray GPhotoFactory::defaultCameraDevice() const
{
    updateDevices();
    return m_defaultCameraDevice;
}

CameraAbilities GPhotoFactory::cameraAbilities(int cameraIndex) const
{
    CameraAbilities abilities;

    if (m_cameraDevices.isEmpty())
        return abilities;

    const QByteArray cameraDevice = m_cameraDevices.values().at(cameraIndex);
    const int abilitiesIndex = gp_abilities_list_lookup_model(m_cameraAbilitiesList, cameraDevice.data());
    if (abilitiesIndex < GP_OK) {
        qWarning() << "GPhoto: unable to find camera abilities";
        return abilities;
    }

    const int ret = gp_abilities_list_get_abilities(m_cameraAbilitiesList, abilitiesIndex, &abilities);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to get camera abilities";
        return abilities;
    }

    return abilities;
}

GPPortInfo GPhotoFactory::portInfo(int cameraIndex) const
{
    GPPortInfo info;
    gp_port_info_new(&info);

    if (m_cameraDevices.isEmpty())
        return info;

    const QByteArray cameraDescription = m_cameraDescriptions.at(cameraIndex).toLatin1();
    const int port = gp_port_info_list_lookup_path(m_portInfoList, cameraDescription.data());
    if (port < GP_OK) {
        qWarning() << "GPhoto: unable to find camera port";
        return info;
    }

    const int ret = gp_port_info_list_get_info (m_portInfoList, port, &info);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to get camera port info";
        return info;
    }

    return info;
}

void GPhotoFactory::initCameraAbilitiesList()
{
    int ret = gp_abilities_list_new(&m_cameraAbilitiesList);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to create camera abilities list";
        return;
    }

    ret = gp_abilities_list_load(m_cameraAbilitiesList, m_context);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to load camera abilities list";
        return;
    }
}

void GPhotoFactory::initPortInfoList()
{
    int ret = gp_port_info_list_new(&m_portInfoList);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to create port info list";
        return;
    }

    ret = gp_port_info_list_load(m_portInfoList);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to load port info list";
        return;
    }

    ret = gp_port_info_list_count(m_portInfoList);
    if (ret < 0) {
        qWarning() << "GPhoto: port info list is empty";
        return;
    }
}

void GPhotoFactory::updateDevices() const
{
    QMutexLocker locker(&m_mutex);

    if (!m_cameraDevices.isEmpty())
        return;

    CameraList *cameraList;
    int ret = gp_list_new(&cameraList);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to create camera list";
        return;
    }

    ret = gp_abilities_list_detect(m_cameraAbilitiesList, m_portInfoList, cameraList, m_context);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to detect abilities list";
        gp_list_free(cameraList);
        return;
    }

    const int cameraCount = gp_list_count(cameraList);
    if (cameraCount < GP_OK) {
        qDebug() << "GPhoto: camera not found";
        gp_list_free(cameraList);
        return;
    }

    QMap<QString, int> deviceNameIndexes;
    for (int i = 0; i < cameraCount; ++i) {
        const char *name, *description;

        ret = gp_list_get_name(cameraList, i, &name);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: unable to get camera name";
            continue;
        }

        ret = gp_list_get_value(cameraList, i, &description);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: unable to get camera description";
            continue;
        }

        qDebug() << "GPhoto: found" << name << "at port" << description;

        QString deviceName = name;
        if (deviceNameIndexes.contains(deviceName))
            deviceName.append(QString(" (%1)").arg(++deviceNameIndexes[deviceName]));
        else
            deviceNameIndexes.insert(deviceName, 0);


        m_cameraDevices.insert(deviceName.toUtf8(), QByteArray(name));
        m_cameraDescriptions.append(QString::fromLatin1(description));
    }

    gp_list_free(cameraList);

    if (!m_cameraDevices.isEmpty())
        m_defaultCameraDevice = m_cameraDevices.first();
}
