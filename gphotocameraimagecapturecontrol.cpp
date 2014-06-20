#include "gphotocameraimagecapturecontrol.h"
#include "gphotocamerasession.h"

GPhotoCameraImageCaptureControl::GPhotoCameraImageCaptureControl(GPhotoCameraSession *session, QObject *parent)
    : QCameraImageCaptureControl(parent)
    , m_session(session)
{
    connect(m_session, SIGNAL(imageCaptureError(int,int,QString)), SIGNAL(error(int,int,QString)));
    connect(m_session, SIGNAL(imageCaptured(int,QImage)), SIGNAL(imageCaptured(int,QImage)));
    connect(m_session, SIGNAL(imageAvailable(int,QVideoFrame)), SIGNAL(imageAvailable(int,QVideoFrame)));
    connect(m_session, SIGNAL(imageSaved(int,QString)), SIGNAL(imageSaved(int,QString)));
    connect(m_session, SIGNAL(readyForCaptureChanged(bool)), SIGNAL(readyForCaptureChanged(bool)));
}

QCameraImageCapture::DriveMode GPhotoCameraImageCaptureControl::driveMode() const
{
    return QCameraImageCapture::SingleImageCapture;
}

void GPhotoCameraImageCaptureControl::setDriveMode(QCameraImageCapture::DriveMode /*driveMode*/)
{
}

bool GPhotoCameraImageCaptureControl::isReadyForCapture() const
{
    return m_session->isReadyForCapture();
}

int GPhotoCameraImageCaptureControl::capture(const QString &fileName)
{
    return m_session->capture(fileName);
}

void GPhotoCameraImageCaptureControl::cancelCapture()
{
}
