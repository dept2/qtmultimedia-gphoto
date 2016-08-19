#ifndef GPHOTOFACTORY_H
#define GPHOTOFACTORY_H

#include <QMutex>
#include <QStringList>
#include <gphoto2/gphoto2-camera.h>

class GPhotoFactory
{
public:
    GPhotoFactory();
    ~GPhotoFactory();

    QList<QByteArray> cameraDevices() const;
    QStringList cameraDescriptions() const;
    QByteArray defaultCameraDevice() const;
    QString cameraDescription(const QByteArray &cameraDevice) const;

    CameraAbilities cameraAbilities(const QByteArray &cameraDevice, bool *ok = 0) const;
    GPPortInfo portInfo(const QString &cameraDescription, bool *ok = 0) const;

private:
    void initCameraAbilitiesList();
    void initPortInfoList();
    void updateDevices() const;

    GPContext *m_context;
    CameraAbilitiesList *m_cameraAbilitiesList;
    GPPortInfoList *m_portInfoList;

    mutable QMutex m_mutex;
    mutable QList<QByteArray> m_cameraDevices;
    mutable QStringList m_cameraDescriptions;
    mutable QByteArray m_defaultCameraDevice;
};

#endif // GPHOTOFACTORY_H
