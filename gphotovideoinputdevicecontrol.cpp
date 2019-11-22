#include "gphotovideoinputdevicecontrol.h"
#include "gphotocamerasession.h"
#include "gphotofactory.h"

#include <QDebug>


GPhotoVideoInputDeviceControl::GPhotoVideoInputDeviceControl(GPhotoFactory *factory, GPhotoCameraSession *session, QObject *parent)
    : QVideoDeviceSelectorControl(parent)
    , m_factory(factory)
    , m_session(session)
{
}

int GPhotoVideoInputDeviceControl::deviceCount() const
{
    return m_factory->cameraNames().size();
}

QString GPhotoVideoInputDeviceControl::deviceName(int index) const
{
    return m_factory->cameraNames().value(index);
}

QString GPhotoVideoInputDeviceControl::deviceDescription(int index) const
{
    return m_factory->cameraNames().value(index);
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
