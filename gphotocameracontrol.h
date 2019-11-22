#ifndef GPHOTOCAMERACONTROL_H
#define GPHOTOCAMERACONTROL_H

#include <QCameraControl>

class GPhotoCameraSession;

class GPhotoCameraControl final : public QCameraControl
{
    Q_OBJECT
public:
    explicit GPhotoCameraControl(GPhotoCameraSession *session, QObject *parent = nullptr);

    GPhotoCameraControl(GPhotoCameraControl&&) = delete;
    GPhotoCameraControl& operator=(GPhotoCameraControl&&) = delete;
    ~GPhotoCameraControl() = default;

    QCamera::State state() const override;
    void setState(QCamera::State state) override;

    QCamera::Status status() const override;

    bool isCaptureModeSupported(QCamera::CaptureModes mode) const override;
    QCamera::CaptureModes captureMode() const override;
    void setCaptureMode(QCamera::CaptureModes captureMode) override;

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const override;

private:
    Q_DISABLE_COPY(GPhotoCameraControl)

    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOCAMERACONTROL_H
