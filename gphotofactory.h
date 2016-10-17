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

    QMap<QByteArray, QByteArray> cameraDevices() const;
    QStringList cameraDescriptions() const;
    QByteArray defaultCameraDevice() const;

    CameraAbilities cameraAbilities(int cameraIndex, bool *ok = 0) const;
    PortInfo portInfo(int cameraIndex, bool *ok = 0) const;

private:
    void initCameraAbilitiesList();
    void updateDevices() const;

    GPContext *const m_context;
    CameraAbilitiesList *m_cameraAbilitiesList;

    mutable QMutex m_mutex;
    mutable QMap<QByteArray, QByteArray> m_cameraDevices;
    mutable QStringList m_cameraDescriptions;
    mutable QByteArray m_defaultCameraDevice;
};

#endif // GPHOTOFACTORY_H
