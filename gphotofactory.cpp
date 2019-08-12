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

QByteArrayList GPhotoFactory::cameraNames() const
{
    updateDevices();
    return m_names;
}

QByteArray GPhotoFactory::defaultCameraName() const
{
    updateDevices();
    return m_defaultCameraName;
}

CameraAbilities GPhotoFactory::cameraAbilities(int cameraIndex, bool *ok) const
{
    updateDevices();
    CameraAbilities abilities;

    if (m_models.isEmpty()) {
        if (ok) *ok = false;
        return abilities;
    }

    const QByteArray &model = m_models.at(cameraIndex);
    const int abilitiesIndex = gp_abilities_list_lookup_model(m_cameraAbilitiesList, model.constData());
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

    if (m_names.isEmpty()) {
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

    const QByteArray &path = m_paths.at(cameraIndex);
    const int port = gp_port_info_list_lookup_path(info.portInfoList, path.data());
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
      m_paths.clear();
      m_models.clear();
      m_names.clear();
      m_defaultCameraName.clear();
    }

    if (!m_names.isEmpty())
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

    QMap<QByteArray, int> displayNameIndexes;
    for (int i = 0; i < cameraCount; ++i) {
        const char *name, *path;

        ret = gp_list_get_name(cameraList.data, i, &name);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: unable to get camera name";
            continue;
        }

        ret = gp_list_get_value(cameraList.data, i, &path);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: unable to get camera path";
            continue;
        }

        QByteArray displayName = QByteArray(name);
        if (displayNameIndexes.contains(name))
            displayName.append(QString(" (%1)").arg(++displayNameIndexes[name]));
        else
            displayNameIndexes.insert(displayName, 0);

//        qDebug() << "GPhoto: found" << qPrintable(displayName) << "at path" << path;

        m_paths.append(QByteArray(path));
        m_models.append(QByteArray(name));
        m_names.append(displayName);
    }

    if (!m_names.isEmpty()) {
        m_defaultCameraName = m_names.first();
        camerasCacheAgeTimer.restart();
    }
}
