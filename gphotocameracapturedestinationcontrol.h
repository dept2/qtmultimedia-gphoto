#ifndef GPHOTOCAMERACAPTUREDESTINATIONCONTROL_H
#define GPHOTOCAMERACAPTUREDESTINATIONCONTROL_H

#include <QCameraCaptureDestinationControl>

class GPhotoCameraSession;

class GPhotoCameraCaptureDestinationControl : public QCameraCaptureDestinationControl
{
    Q_OBJECT
public:
    explicit GPhotoCameraCaptureDestinationControl(GPhotoCameraSession *session, QObject *parent = 0);

    bool isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations destination) const Q_DECL_OVERRIDE;
    QCameraImageCapture::CaptureDestinations captureDestination() const Q_DECL_OVERRIDE;
    void setCaptureDestination(QCameraImageCapture::CaptureDestinations destination) Q_DECL_OVERRIDE;

private:
    GPhotoCameraSession *m_session;
};

#endif // GPHOTOCAMERACAPTUREDESTINATIONCONTROL_H
