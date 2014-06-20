#ifndef GPHOTOCAMERACONTROL_H
#define GPHOTOCAMERACONTROL_H

#include <QCameraControl>

class GPhotoCameraSession;

class GPhotoCameraControl : public QCameraControl
{
    Q_OBJECT
public:
    explicit GPhotoCameraControl(GPhotoCameraSession *session, QObject *parent = 0);

    QCamera::State state() const Q_DECL_OVERRIDE;
    void setState(QCamera::State state) Q_DECL_OVERRIDE;

    QCamera::Status status() const Q_DECL_OVERRIDE;

    bool isCaptureModeSupported(QCamera::CaptureModes mode) const Q_DECL_OVERRIDE;
    QCamera::CaptureModes captureMode() const Q_DECL_OVERRIDE;
    void setCaptureMode(QCamera::CaptureModes captureMode) Q_DECL_OVERRIDE;

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const;

private:
    GPhotoCameraSession *m_session;

};

#endif // GPHOTOCAMERACONTROL_H
