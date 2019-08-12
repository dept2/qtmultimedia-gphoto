#ifndef GPHOTOFACTORY_H
#define GPHOTOFACTORY_H

#include <QMap>
#include <QMutex>
#include <QStringList>
#include <gphoto2/gphoto2-camera.h>

struct PortInfo
{
    PortInfo();

    GPPortInfo portInfo;
    GPPortInfoList *portInfoList;
};

class GPhotoFactory
{
public:
    GPhotoFactory();
    ~GPhotoFactory();

    QByteArrayList cameraNames() const;
    QByteArray defaultCameraName() const;

    CameraAbilities cameraAbilities(int cameraIndex, bool *ok = 0) const;
    PortInfo portInfo(int cameraIndex, bool *ok = 0) const;

private:
    void initCameraAbilitiesList();
    void updateDevices() const;

    GPContext *const m_context;
    CameraAbilitiesList *m_cameraAbilitiesList;

    mutable QMutex m_mutex;
    mutable QByteArrayList m_paths;
    mutable QByteArrayList m_models;
    mutable QByteArrayList m_names;
    mutable QByteArray m_defaultCameraName;
};

#endif // GPHOTOFACTORY_H
