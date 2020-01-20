#include <functional>

#include <QDebug>
#include <QEventLoop>
#include <QThread>

#include <gphoto2/gphoto2-list.h>
#include <gphoto2/gphoto2-port-result.h>

#include "gphotocamera.h"
#include "gphotoworker.h"

namespace {
    constexpr auto deviceCacheLifetime = 1000;
}

using CameraListPtr = std::unique_ptr<CameraList, int (*)(CameraList*)>;

GPhotoDevices::GPhotoDevices()
    : portInfoList(nullptr, gp_port_info_list_free)
{
    GPPortInfoList *list;
    gp_port_info_list_new(&list);
    portInfoList.reset(list);
}

GPhotoWorker::GPhotoWorker()
    : m_context(nullptr, gp_context_unref)
    , m_abilitiesList(nullptr, gp_abilities_list_free)
{
}

GPhotoWorker::~GPhotoWorker()
{
}

bool GPhotoWorker::init()
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

QList<QByteArray> GPhotoWorker::cameraNames()
{
    updateDevices();
    return m_devices.names;
}

QByteArray GPhotoWorker::defaultCameraName()
{
    updateDevices();
    return m_devices.defaultCameraName;
}

void GPhotoWorker::initCamera(int cameraIndex)
{
    if (m_cameras.cend() != m_cameras.find(cameraIndex))
        return;

    updateDevices();

    auto ok = false;

    const auto &abilities = getCameraAbilities(cameraIndex, &ok);
    if (!ok) {
        qWarning() << "Unable to get abilities for camera with index" << cameraIndex;
        return;
    }

    const auto &portInfo = getPortInfo(cameraIndex, &ok);
    if (!ok) {
        qWarning() << "Unable to get port info for camera with index" << cameraIndex;
        return;
    }

    auto camera = new GPhotoCamera(m_context.get(), abilities, portInfo, this);

    using Camera = GPhotoCamera;
    using Worker = GPhotoWorker;
    using namespace std::placeholders;

    connect(camera, &Camera::captureModeChanged, this, std::bind(&Worker::captureModeChanged, this, cameraIndex, _1));
    connect(camera, &Camera::error, this, std::bind(&Worker::error, this, cameraIndex, _1, _2));
    connect(camera, &Camera::imageCaptureError, this, std::bind(&Worker::imageCaptureError, this, cameraIndex, _1, _2, _3));
    connect(camera, &Camera::imageCaptured, this, std::bind(&Worker::imageCaptured, this, cameraIndex, _1, _2, _3, _4));
    connect(camera, &Camera::previewCaptured, this, std::bind(&Worker::previewCaptured, this, cameraIndex, _1));
    connect(camera, &Camera::readyForCaptureChanged, this, std::bind(&Worker::readyForCaptureChanged, this, cameraIndex, _1));
    connect(camera, &Camera::stateChanged, this, std::bind(&Worker::stateChanged, this, cameraIndex, _1));
    connect(camera, &Camera::statusChanged, this, std::bind(&Worker::statusChanged, this, cameraIndex, _1));

    m_cameras.emplace(std::make_pair(cameraIndex, camera));
}

void GPhotoWorker::setState(int cameraIndex, QCamera::State state)
{
    m_cameras.at(cameraIndex)->setState(state);
}

void GPhotoWorker::setCaptureMode(int cameraIndex, QCamera::CaptureModes captureMode)
{
    m_cameras.at(cameraIndex)->setCaptureMode(captureMode);
}

void GPhotoWorker::capturePhoto(int cameraIndex, int id, const QString &fileName)
{
    m_cameras.at(cameraIndex)->capturePhoto(id, fileName);
}

QVariant GPhotoWorker::parameter(int cameraIndex, const QString &name)
{
    return m_cameras.at(cameraIndex)->parameter(name);
}

bool GPhotoWorker::setParameter(int cameraIndex, const QString &name, const QVariant &value)
{
    return m_cameras.at(cameraIndex)->setParameter(name, value);
}

QVariantList GPhotoWorker::parameterValues(int cameraIndex, const QString &name, QMetaType::Type valueType) const
{
    return m_cameras.at(cameraIndex)->parameterValues(name, valueType);
}

CameraAbilities GPhotoWorker::getCameraAbilities(int cameraIndex, bool *ok)
{
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

GPPortInfo GPhotoWorker::getPortInfo(int cameraIndex, bool *ok)
{
    GPPortInfo info;
    gp_port_info_new(&info);

    if (m_devices.names.isEmpty()) {
        if (ok) *ok = false;
        return info;
    }

    const auto &path = m_devices.paths.at(cameraIndex);
    auto port = gp_port_info_list_lookup_path(m_devices.portInfoList.get(), path.constData());
    if (port < GP_OK) {
        qWarning() << "GPhoto: unable to find camera port";
        if (ok) *ok = false;
        return info;
    }


    auto ret = gp_port_info_list_get_info(m_devices.portInfoList.get(), port, &info);
    if (ret < GP_OK) {
        qWarning() << "GPhoto: unable to get camera port info";
        if (ok) *ok = false;
        return info;
    }

    if (ok) *ok = true;
    return info;
}

void GPhotoWorker::updateDevices()
{
    QMutexLocker locker(&m_mutex);

    if (m_devices.cacheAgeTimer.isValid() && deviceCacheLifetime < m_devices.cacheAgeTimer.elapsed()) {
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
    for (auto i = 0; i < cameraCount; ++i) {
        const char *name = nullptr;
        const char *path = nullptr;

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
