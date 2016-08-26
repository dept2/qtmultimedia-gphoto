#ifndef GPHOTOVIDEOINPUTDEVICECONTROL_H
#define GPHOTOVIDEOINPUTDEVICECONTROL_H

#include <QVideoDeviceSelectorControl>

class GPhotoFactory;
class GPhotoCameraSession;

class GPhotoVideoInputDeviceControl : public QVideoDeviceSelectorControl
{
public:
    GPhotoVideoInputDeviceControl(GPhotoFactory *factory, GPhotoCameraSession *session, QObject *parent = 0);

    int deviceCount() const;
    QString deviceName(int index) const;
    QString deviceDescription(int index) const;
    int defaultDevice() const;
    int selectedDevice() const;

public slots:
    void setSelectedDevice(int index);

private:
    int m_selectedDevice;
    GPhotoFactory *const m_factory;
    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOVIDEOINPUTDEVICECONTROL_H
