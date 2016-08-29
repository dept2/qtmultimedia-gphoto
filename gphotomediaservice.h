#ifndef GPHOTOMEDIASERVICE_H
#define GPHOTOMEDIASERVICE_H

#include <QMediaService>

class GPhotoCameraSession;
class GPhotoCameraControl;
class GPhotoVideoRendererControl;
class GPhotoCameraImageCaptureControl;
class GPhotoCameraCaptureDestinationControl;
class GPhotoVideoProbeControl;
class GPhotoExposureControl;
class GPhotoFactory;
class GPhotoVideoInputDeviceControl;

class GPhotoMediaService : public QMediaService
{
    Q_OBJECT

public:
    explicit GPhotoMediaService(GPhotoFactory *factory, QObject *parent = 0);

    QMediaControl* requestControl(const char *name);
    void releaseControl(QMediaControl *control);

private:
    GPhotoCameraSession *m_session;
    GPhotoCameraControl *m_cameraControl;
    GPhotoVideoRendererControl *m_videoRendererControl;
    GPhotoCameraImageCaptureControl *m_imageCaptureControl;
    GPhotoCameraCaptureDestinationControl *m_destinationControl;
    GPhotoVideoProbeControl *m_videoProbeControl;
    GPhotoExposureControl *m_exposureControl;
    GPhotoVideoInputDeviceControl *m_videoInputDeviceControl;
};

#endif // GPHOTOMEDIASERVICE_H
