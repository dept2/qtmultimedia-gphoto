#ifndef GPHOTOVIDEOINPUTDEVICECONTROL_H
#define GPHOTOVIDEOINPUTDEVICECONTROL_H

#include <QVideoDeviceSelectorControl>

class GPhotoFactory;
class GPhotoCameraSession;

class GPhotoVideoInputDeviceControl : public QVideoDeviceSelectorControl
{
public:
    GPhotoVideoInputDeviceControl(GPhotoFactory *factory, GPhotoCameraSession *session, QObject *parent = 0);

    int deviceCount() const Q_DECL_OVERRIDE;

    QString deviceName(int index) const Q_DECL_OVERRIDE;
    QString deviceDescription(int index) const Q_DECL_OVERRIDE;

    int defaultDevice() const Q_DECL_OVERRIDE;
    int selectedDevice() const Q_DECL_OVERRIDE;

public slots:
    void setSelectedDevice(int index) Q_DECL_OVERRIDE;

private:
    int m_selectedDevice;
    GPhotoFactory *const m_factory;
    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOVIDEOINPUTDEVICECONTROL_H
