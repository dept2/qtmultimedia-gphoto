#include "gphotovideoinputdevicecontrol.h"
#include "gphotocamerasession.h"
#include "gphotofactory.h"

#include <QDebug>


GPhotoVideoInputDeviceControl::GPhotoVideoInputDeviceControl(GPhotoFactory* photoFactory, GPhotoCameraSession* session, QObject* parent)
  : QVideoDeviceSelectorControl(parent)
  , m_selectedDevice(0)
  , m_factory(photoFactory)
  , m_session(session)
{
}

int GPhotoVideoInputDeviceControl::deviceCount() const
{
    return m_factory->cameraDevices().size();
}

QString GPhotoVideoInputDeviceControl::deviceName(int index) const
{
    return QString::fromUtf8(m_factory->cameraDevices().at(index));
}

QString GPhotoVideoInputDeviceControl::deviceDescription(int index) const
{
    return m_factory->cameraDescriptions().at(index);
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
