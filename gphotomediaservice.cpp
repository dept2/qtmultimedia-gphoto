#include "gphotomediaservice.h"
#include "gphotocamerasession.h"
#include "gphotocameracontrol.h"
#include "gphotovideorenderercontrol.h"
#include "gphotocameraimagecapturecontrol.h"
#include "gphotocameracapturedestinationcontrol.h"


GPhotoMediaService::GPhotoMediaService(QObject *parent)
    : QMediaService(parent)
    , m_session(new GPhotoCameraSession(this))
    , m_cameraControl(new GPhotoCameraControl(m_session, this))
    , m_videoRendererControl(new GPhotoVideoRendererControl(m_session, this))
    , m_imageCaptureControl(new GPhotoCameraImageCaptureControl(m_session, this))
    , m_destinationControl(new GPhotoCameraCaptureDestinationControl(m_session, this))
{
}

QMediaControl *GPhotoMediaService::requestControl(const char *name)
{
    if (qstrcmp(name, QCameraControl_iid) == 0)
        return m_cameraControl;
    else if (qstrcmp(name, QVideoRendererControl_iid) == 0)
        return m_videoRendererControl;
    else if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return m_imageCaptureControl;
    else if (qstrcmp(name, QCameraCaptureDestinationControl_iid) == 0)
        return m_destinationControl;

    qWarning("gphoto control %s not implemented yet", name);
    return 0;
}

void GPhotoMediaService::releaseControl(QMediaControl *control)
{
    Q_UNUSED(control);
}
