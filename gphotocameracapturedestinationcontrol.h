#ifndef GPHOTOCAMERACAPTUREDESTINATIONCONTROL_H
#define GPHOTOCAMERACAPTUREDESTINATIONCONTROL_H

#include <QCameraCaptureDestinationControl>

class GPhotoCameraSession;

class GPhotoCameraCaptureDestinationControl final : public QCameraCaptureDestinationControl
{
    Q_OBJECT
public:
    explicit GPhotoCameraCaptureDestinationControl(GPhotoCameraSession *session, QObject *parent = nullptr);

    GPhotoCameraCaptureDestinationControl(GPhotoCameraCaptureDestinationControl&&) = delete;
    GPhotoCameraCaptureDestinationControl& operator=(GPhotoCameraCaptureDestinationControl&&) = delete;
    ~GPhotoCameraCaptureDestinationControl() = default;

    bool isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations destination) const override;
    QCameraImageCapture::CaptureDestinations captureDestination() const override;
    void setCaptureDestination(QCameraImageCapture::CaptureDestinations destination) override;

private:
    Q_DISABLE_COPY(GPhotoCameraCaptureDestinationControl)

    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOCAMERACAPTUREDESTINATIONCONTROL_H
