#ifndef GPHOTOVIDEOINPUTDEVICECONTROL_H
#define GPHOTOVIDEOINPUTDEVICECONTROL_H

#include <QVideoDeviceSelectorControl>

class GPhotoFactory;
class GPhotoCameraSession;

class GPhotoVideoInputDeviceControl final : public QVideoDeviceSelectorControl
{
    Q_OBJECT
public:
    GPhotoVideoInputDeviceControl(GPhotoFactory *factory, GPhotoCameraSession *session, QObject *parent = nullptr);

    GPhotoVideoInputDeviceControl(GPhotoVideoInputDeviceControl&&) = delete;
    GPhotoVideoInputDeviceControl& operator=(GPhotoVideoInputDeviceControl&&) = delete;
    ~GPhotoVideoInputDeviceControl() = default;

    int deviceCount() const override;

    QString deviceName(int index) const override;
    QString deviceDescription(int index) const override;

    int defaultDevice() const override;
    int selectedDevice() const override;

public slots:
    void setSelectedDevice(int index) override;

private:
    Q_DISABLE_COPY(GPhotoVideoInputDeviceControl)

    int m_selectedDevice = -1;
    GPhotoFactory *const m_factory;
    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOVIDEOINPUTDEVICECONTROL_H
