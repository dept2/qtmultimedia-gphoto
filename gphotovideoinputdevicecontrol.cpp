#include "gphotovideoinputdevicecontrol.h"
#include "gphotocamerasession.h"
#include "gphotofactory.h"

#include <QDebug>


GPhotoVideoInputDeviceControl::GPhotoVideoInputDeviceControl(GPhotoFactory* factory, GPhotoCameraSession* session, QObject* parent)
    : QVideoDeviceSelectorControl(parent)
    , m_selectedDevice(-1)
    , m_factory(factory)
    , m_session(session)
{
}

int GPhotoVideoInputDeviceControl::deviceCount() const
{
    return m_factory->cameraDevices().size();
}

QString GPhotoVideoInputDeviceControl::deviceName(int index) const
{
    const QByteArrayList &devices = m_factory->cameraDevices().keys();
    return QString::fromUtf8(devices.empty() ? "" : devices.at(index));
}

QString GPhotoVideoInputDeviceControl::deviceDescription(int index) const
{
    const QStringList &descriptions = m_factory->cameraDescriptions();
    return descriptions.isEmpty() ? "" : descriptions.at(index);
}

int GPhotoVideoInputDeviceControl::defaultDevice() const
{
    return 0;
}

int GPhotoVideoInputDeviceControl::selectedDevice() const
{
    return m_selectedDevice;
}

void GPhotoVideoInputDeviceControl::setSelectedDevice(int index)
{
    if (index != m_selectedDevice) {
        m_selectedDevice = index;
        m_session->setCamera(index);
        emit selectedDeviceChanged(index);
        emit selectedDeviceChanged(deviceName(index));
    }
}
