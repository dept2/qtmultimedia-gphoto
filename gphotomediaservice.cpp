#include "gphotomediaservice.h"
#include "gphotocamerasession.h"
#include "gphotocameracontrol.h"
#include "gphotovideorenderercontrol.h"
#include "gphotocameraimagecapturecontrol.h"
#include "gphotocameracapturedestinationcontrol.h"
#include "gphotovideoprobecontrol.h"
#include "gphotoexposurecontrol.h"
#include "gphotovideoinputdevicecontrol.h"

GPhotoMediaService::GPhotoMediaService(GPhotoFactory *factory, QObject *parent)
    : QMediaService(parent)
    , m_session(new GPhotoCameraSession(factory, this))
    , m_cameraControl(new GPhotoCameraControl(m_session, this))
    , m_videoRendererControl(new GPhotoVideoRendererControl(m_session, this))
    , m_imageCaptureControl(new GPhotoCameraImageCaptureControl(m_session, this))
    , m_destinationControl(new GPhotoCameraCaptureDestinationControl(m_session, this))
    , m_videoProbeControl(new GPhotoVideoProbeControl(m_session, this))
    , m_exposureControl(new GPhotoExposureControl(m_session, this))
    , m_videoInputDeviceControl(new GPhotoVideoInputDeviceControl(factory, m_session, this))
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
    else if (qstrcmp(name, QMediaVideoProbeControl_iid) == 0)
        return m_videoProbeControl;
    else if (qstrcmp(name, QCameraExposureControl_iid) == 0)
        return m_exposureControl;
    else if (qstrcmp(name, QVideoDeviceSelectorControl_iid) == 0)
        return m_videoInputDeviceControl;

    return 0;
}

void GPhotoMediaService::releaseControl(QMediaControl *control)
{
    Q_UNUSED(control);
}
