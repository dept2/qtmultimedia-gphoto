#ifndef GPHOTOFACTORY_H
#define GPHOTOFACTORY_H

#include <QMap>
#include <QMutex>
#include <QStringList>
#include <gphoto2/gphoto2-camera.h>

class GPhotoFactory
{
public:
    GPhotoFactory();
    ~GPhotoFactory();

    QMap<QByteArray, QByteArray> cameraDevices() const;
    QStringList cameraDescriptions() const;
    QByteArray defaultCameraDevice() const;

    CameraAbilities cameraAbilities(int cameraIndex) const;
    GPPortInfo portInfo(int cameraIndex) const;

private:
    void initCameraAbilitiesList();
    void initPortInfoList();
    void updateDevices() const;

    GPContext *const m_context;
    CameraAbilitiesList *m_cameraAbilitiesList;
    GPPortInfoList *m_portInfoList;

    mutable QMutex m_mutex;
    mutable QMap<QByteArray, QByteArray> m_cameraDevices;
    mutable QStringList m_cameraDescriptions;
    mutable QByteArray m_defaultCameraDevice;
};

#endif // GPHOTOFACTORY_H
