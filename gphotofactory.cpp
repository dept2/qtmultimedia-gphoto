#include "gphotofactory.h"

#include <QDebug>

Devices::Devices()
    : portInfoList(nullptr, gp_port_info_list_free)
{
    GPPortInfoList *list;
    gp_port_info_list_new(&list);
    portInfoList.reset(list);
}

GPhotoFactory::GPhotoFactory()
    : m_context(nullptr, gp_context_unref)
    , m_abilitiesList(nullptr, gp_abilities_list_free)
{}

bool GPhotoFactory::init()
{
    m_context.reset(gp_context_new());

    if (!m_context) {
        qWarning() << "GPhoto: unable to create GPhoto context";
        return false;
    }

    CameraAbilitiesList *list;
    gp_abilities_list_new(&list);
    m_abilitiesList.reset(list);

    auto ret = gp_abilities_list_load(list, m_context.get());
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to load camera abilities list";
        return false;
    }

    ret = gp_abilities_list_count(list);
    if (ret < 1) {
        qWarning() << "GPhoto: camera abilities list is empty";
        return false;
    }

    return true;
}

QByteArrayList GPhotoFactory::cameraNames() const
{
    updateDevices();
    return m_devices.names;
}

QByteArray GPhotoFactory::defaultCameraName() const
{
    updateDevices();
    return m_devices.defaultCameraName;
}


GPContext* GPhotoFactory::context() const
{
    return m_context.get();
}

CameraAbilities GPhotoFactory::cameraAbilities(int cameraIndex, bool *ok) const
{
    updateDevices();
    CameraAbilities abilities;

    if (m_devices.models.isEmpty()) {
        if (ok) *ok = false;
        return abilities;
    }

    const auto &model = m_devices.models.at(cameraIndex);
    auto abilitiesIndex = gp_abilities_list_lookup_model(m_abilitiesList.get(), model.constData());
    if (abilitiesIndex < GP_OK) {
        qWarning() << "GPhoto: unable to find camera abilities";
        if (ok) *ok = false;
        return abilities;
    }

    auto ret = gp_abilities_list_get_abilities(m_abilitiesList.get(), abilitiesIndex, &abilities);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to get camera abilities";
        if (ok) *ok = false;
        return abilities;
    }

    if (ok) *ok = true;
    return abilities;
}

GPPortInfo GPhotoFactory::portInfo(int cameraIndex, bool *ok) const
{
    updateDevices();

    if (m_devices.names.isEmpty()) {
        if (ok) *ok = false;
        return GPPortInfo();
    }

    QMutexLocker locker(&m_mutex);

    const auto &path = m_devices.paths.at(cameraIndex);
    auto port = gp_port_info_list_lookup_path(m_devices.portInfoList.get(), path.constData());
    if (port < GP_OK) {
        qWarning() << "GPhoto: unable to find camera port";
        if (ok) *ok = false;
        return GPPortInfo();
    }

    GPPortInfo info;
    gp_port_info_new(&info);

    auto ret = gp_port_info_list_get_info(m_devices.portInfoList.get(), port, &info);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to get camera port info";
        if (ok) *ok = false;
        return info;
    }

    if (ok) *ok = true;
    return info;
}

void GPhotoFactory::updateDevices() const
{
    QMutexLocker locker(&m_mutex);

    if (m_devices.cacheAgeTimer.isValid() && 1000 < m_devices.cacheAgeTimer.elapsed()) {
        m_devices.paths.clear();
        m_devices.models.clear();
        m_devices.names.clear();
        m_devices.defaultCameraName.clear();
    }

    if (!m_devices.names.isEmpty())
        return;

    auto ret = gp_port_info_list_load(m_devices.portInfoList.get());
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to load port info list";
        return;
    }

    ret = gp_port_info_list_count(m_devices.portInfoList.get());
    if (ret < 1) {
        qWarning() << "GPhoto: port info list is empty";
        return;
    }

    CameraList *cameraList;
    gp_list_new(&cameraList);

    // Unique pointer will free memory on exit
    auto cameraListPtr = CameraListPtr(cameraList, gp_list_free);

    ret = gp_abilities_list_detect(m_abilitiesList.get(), m_devices.portInfoList.get(), cameraList, m_context.get());
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to detect abilities list";
        return;
    }

    auto cameraCount = gp_list_count(cameraList);
    if (cameraCount < 1)
        return;

    QMap<QByteArray, int> displayNameIndexes;
    for (int i = 0; i < cameraCount; ++i) {
        const char *name, *path;

        ret = gp_list_get_name(cameraList, i, &name);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: unable to get camera name";
            continue;
        }

        ret = gp_list_get_value(cameraList, i, &path);
        if (ret < GP_OK) {
            qWarning() << "GPhoto: unable to get camera path";
            continue;
        }

        auto displayName = QByteArray(name);
        if (displayNameIndexes.contains(name))
            displayName.append(QString(" (%1)").arg(++displayNameIndexes[name]));
        else
            displayNameIndexes.insert(displayName, 0);

//        qDebug() << "GPhoto: found" << qPrintable(displayName) << "at path" << path;

        m_devices.paths.append(QByteArray(path));
        m_devices.models.append(QByteArray(name));
        m_devices.names.append(displayName);
    }

    if (!m_devices.names.isEmpty()) {
        m_devices.defaultCameraName = m_devices.names.first();
        m_devices.cacheAgeTimer.restart();
    }
}
