#ifndef GPHOTOFACTORY_H
#define GPHOTOFACTORY_H

#include <memory>

#include <QElapsedTimer>
#include <QMap>
#include <QMutex>
#include <QStringList>

#include <gphoto2/gphoto2-camera.h>

using CameraAbilitiesListPtr = std::unique_ptr<CameraAbilitiesList, int (*)(CameraAbilitiesList*)>;
using CameraListPtr = std::unique_ptr<CameraList, int (*)(CameraList*)>;
using GPContextPtr = std::unique_ptr<GPContext, void (*)(GPContext*)>;
using GPPortInfoListPtr = std::unique_ptr<GPPortInfoList, int (*)(GPPortInfoList*)>;

struct Devices final
{
    Devices();

    QList<QByteArray> paths;
    QList<QByteArray> models;
    QList<QByteArray> names;
    QByteArray defaultCameraName;
    GPPortInfoListPtr portInfoList;
    QElapsedTimer cacheAgeTimer;
};

class GPhotoFactory final
{
public:
    GPhotoFactory();

    bool init();

    QList<QByteArray> cameraNames() const;
    QByteArray defaultCameraName() const;

    GPContext* context() const;
    CameraAbilities cameraAbilities(int cameraIndex, bool *ok = nullptr) const;
    GPPortInfo portInfo(int cameraIndex, bool *ok = nullptr) const;

private:
    void updateDevices() const;

    GPContextPtr m_context;
    CameraAbilitiesListPtr m_abilitiesList;
    mutable Devices m_devices;
    mutable QMutex m_mutex;
};

#endif // GPHOTOFACTORY_H
