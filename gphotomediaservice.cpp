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
    , m_factory(factory)
    , m_session(new GPhotoCameraSession(factory, this))
{
}

GPhotoMediaService::~GPhotoMediaService()
{
}

QMediaControl *GPhotoMediaService::requestControl(const char *name)
{
    if (qstrcmp(name, QCameraControl_iid) == 0)
        return new GPhotoCameraControl(m_session.get(), this);

    if (qstrcmp(name, QVideoRendererControl_iid) == 0)
        return new GPhotoVideoRendererControl(m_session.get(), this);

    if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return new GPhotoCameraImageCaptureControl(m_session.get(), this);

    if (qstrcmp(name, QCameraCaptureDestinationControl_iid) == 0)
        return new GPhotoCameraCaptureDestinationControl(m_session.get(), this);

    if (qstrcmp(name, QMediaVideoProbeControl_iid) == 0)
        return new GPhotoVideoProbeControl(m_session.get(), this);

    if (qstrcmp(name, QCameraExposureControl_iid) == 0)
        return new GPhotoExposureControl(m_session.get(), this);

    if (qstrcmp(name, QVideoDeviceSelectorControl_iid) == 0)
        return new GPhotoVideoInputDeviceControl(m_factory, m_session.get(), this);

    return nullptr;
}

void GPhotoMediaService::releaseControl(QMediaControl *control)
{
    delete control;
}
