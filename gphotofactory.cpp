#include "gphotofactory.h"

#include <QDebug>
#include <QElapsedTimer>


PortInfo::PortInfo()
{
  gp_port_info_new(&portInfo);
}

struct PortList
{
    PortList() { gp_port_info_list_new(&data); }
    ~PortList() { gp_port_info_list_free(data); }
    GPPortInfoList *data;
};

struct CamList
{
    CamList() { gp_list_new(&data); }
    ~CamList() { gp_list_free(data); }
    CameraList *data;
};

GPhotoFactory::GPhotoFactory()
    : m_context(gp_context_new())
    , m_cameraAbilitiesList(0)
{
    if (!m_context) {
        qWarning() << "Unable to create GPhoto context";
        return;
    }

    initCameraAbilitiesList();
}

GPhotoFactory::~GPhotoFactory()
{
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

CameraAbilities GPhotoFactory::cameraAbilities(int cameraIndex, bool *ok) const
{
    updateDevices();
    CameraAbilities abilities;

    if (m_cameraDevices.isEmpty()) {
        if (ok) *ok = false;
        return abilities;
    }

    const QByteArray &cameraDevice = m_cameraDevices.values().at(cameraIndex);
    const int abilitiesIndex = gp_abilities_list_lookup_model(m_cameraAbilitiesList, cameraDevice.data());
    if (abilitiesIndex < GP_OK) {
        qWarning() << "GPhoto: unable to find camera abilities";
        if (ok) *ok = false;
        return abilities;
    }

    const int ret = gp_abilities_list_get_abilities(m_cameraAbilitiesList, abilitiesIndex, &abilities);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to get camera abilities";
        if (ok) *ok = false;
        return abilities;
    }

    if (ok) *ok = true;
    return abilities;
}

PortInfo GPhotoFactory::portInfo(int cameraIndex, bool *ok) const
{
    updateDevices();

    PortInfo info;

    if (m_cameraDevices.isEmpty()) {
        if (ok) *ok = false;
        return info;
    }

    QMutexLocker locker(&m_mutex);
    gp_port_info_list_new(&info.portInfoList);
    int ret = gp_port_info_list_load(info.portInfoList);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to load port info list";
        gp_port_info_list_free(info.portInfoList);
        if (ok) *ok = false;
        return info;
    }

    ret = gp_port_info_list_count(info.portInfoList);
    if (ret < 1) {
        qWarning() << "GPhoto: port info list is empty";
        gp_port_info_list_free(info.portInfoList);
        if (ok) *ok = false;
        return info;
    }

    const QByteArray &cameraDescription = m_cameraDescriptions.at(cameraIndex).toLatin1();
    const int port = gp_port_info_list_lookup_path(info.portInfoList, cameraDescription.data());
    if (port < GP_OK) {
        qWarning() << "GPhoto: unable to find camera port";
        gp_port_info_list_free(info.portInfoList);
        if (ok) *ok = false;
        return info;
    }

    ret = gp_port_info_list_get_info(info.portInfoList, port, &info.portInfo);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to get camera port info";
        gp_port_info_list_free(info.portInfoList);
        if (ok) *ok = false;
        return info;
    }

    if (ok) *ok = true;
    return info;
}

void GPhotoFactory::initCameraAbilitiesList()
{
    gp_abilities_list_new(&m_cameraAbilitiesList);

    int ret = gp_abilities_list_load(m_cameraAbilitiesList, m_context);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to load camera abilities list";
        return;
    }

    ret = gp_abilities_list_count(m_cameraAbilitiesList);
    if (ret < 1) {
        qWarning() << "GPhoto: camera abilities list is empty";
        return;
    }
}

void GPhotoFactory::updateDevices() const
{
    QMutexLocker locker(&m_mutex);

    static QElapsedTimer camerasCacheAgeTimer;
    if (camerasCacheAgeTimer.isValid() && camerasCacheAgeTimer.elapsed() > 1000) {
      m_cameraDevices.clear();
      m_cameraDescriptions.clear();
      m_defaultCameraDevice.clear();
    }

    if (!m_cameraDevices.isEmpty())
        return;

    PortList portInfoList;
    int ret = gp_port_info_list_load(portInfoList.data);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to load port info list";
        return;
    }

    ret = gp_port_info_list_count(portInfoList.data);
    if (ret < 1) {
        qWarning() << "GPhoto: port info list is empty";
        return;
    }

    CamList cameraList;
    ret = gp_abilities_list_detect(m_cameraAbilitiesList, portInfoList.data, cameraList.data, m_context);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to detect abilities list";
        return;
    }

    const int cameraCount = gp_list_count(cameraList.data);
    if (cameraCount < 1)
        return;

    QMap<QString, int> deviceNameIndexes;
    for (int i = 0; i < cameraCount; ++i) {
        const char *name, *description;

        ret = gp_list_get_name(cameraList.data, i, &name);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: unable to get camera name";
            continue;
        }

        ret = gp_list_get_value(cameraList.data, i, &description);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: unable to get camera description";
            continue;
        }

        //qDebug() << "GPhoto: found" << name << "at port" << description;

        QString deviceName = name;
        if (deviceNameIndexes.contains(deviceName))
            deviceName.append(QString(" (%1)").arg(++deviceNameIndexes[deviceName]));
        else
            deviceNameIndexes.insert(deviceName, 0);


        m_cameraDevices.insert(deviceName.toUtf8(), QByteArray(name));
        m_cameraDescriptions.append(QString::fromLatin1(description));
    }

    if (!m_cameraDevices.isEmpty()) {
        m_defaultCameraDevice = m_cameraDevices.first();
        camerasCacheAgeTimer.restart();
    }
}
