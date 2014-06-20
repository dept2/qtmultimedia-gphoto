#include "gphotocameracapturedestinationcontrol.h"
#include "gphotocamerasession.h"

GPhotoCameraCaptureDestinationControl::GPhotoCameraCaptureDestinationControl(GPhotoCameraSession *session, QObject *parent)
    : QCameraCaptureDestinationControl(parent)
    , m_session(session)
{
    connect(m_session, SIGNAL(captureDestinationChanged(QCameraImageCapture::CaptureDestinations)),
            SIGNAL(captureDestinationChanged(QCameraImageCapture::CaptureDestinations)));
}

bool GPhotoCameraCaptureDestinationControl::isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations destination) const
{
    return m_session->isCaptureDestinationSupported(destination);
}

QCameraImageCapture::CaptureDestinations GPhotoCameraCaptureDestinationControl::captureDestination() const
{
    return m_session->captureDestination();
}

void GPhotoCameraCaptureDestinationControl::setCaptureDestination(QCameraImageCapture::CaptureDestinations destination)
{
    m_session->setCaptureDestination(destination);
}
