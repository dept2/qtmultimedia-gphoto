#ifndef GPHOTOCAMERASESSION_H
#define GPHOTOCAMERASESSION_H

#include <QObject>
#include <QCamera>
#include <QAbstractVideoSurface>
#include <QCameraImageCapture>
#include <QMutex>
#include <QPointer>

#include <gphoto2/gphoto2-camera.h>

class GPhotoCameraWorker;


class GPhotoCameraSession : public QObject
{
    Q_OBJECT
public:
    explicit GPhotoCameraSession(QObject *parent = 0);
    ~GPhotoCameraSession();

    // camera control
    QCamera::State state() const;
    void setState(QCamera::State state);
    QCamera::Status status() const;

    bool isCaptureModeSupported(QCamera::CaptureModes mode) const;
    QCamera::CaptureModes captureMode() const;
    void setCaptureMode(QCamera::CaptureModes captureMode);

    // destination control
    bool isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations destination) const;
    QCameraImageCapture::CaptureDestinations captureDestination() const;
    void setCaptureDestination(QCameraImageCapture::CaptureDestinations destination);

    // capture control
    bool isReadyForCapture() const;
    int capture(const QString& fileName);

    // video renderer control
    QAbstractVideoSurface *surface() const;
    void setSurface(QAbstractVideoSurface *surface);

signals:
    // camera control
    void statusChanged(QCamera::Status);
    void stateChanged(QCamera::State);
    void error(int error, const QString &errorString);
    void captureModeChanged(QCamera::CaptureModes);

    // capture destination control
    void captureDestinationChanged(QCameraImageCapture::CaptureDestinations destination);

    // image capture control
    void readyForCaptureChanged(bool);
//    void imageExposed(int id);
    void imageCaptured(int id, const QImage &preview);
//    void imageMetadataAvailable(int id, const QString &key, const QVariant &value);
    void imageAvailable(int id, const QVideoFrame &buffer);
    void imageSaved(int id, const QString &fileName);
    void imageCaptureError(int id, int error, const QString &errorString);

    // video probe control
    void videoFrameProbed(const QVideoFrame& frame);

private slots:
    void previewCaptured(const QImage& image);
    void imageDataCaptured(int id, const QByteArray& imageData, const QString& fileName);

private:
    bool openCamera();
    void closeCamera();

    bool startViewFinder();
    void stopViewFinder();

    QCamera::State m_state;
    QCamera::Status m_status;
    QCamera::CaptureModes m_captureMode;

    QCameraImageCapture::CaptureDestinations m_captureDestination;

    QPointer<QAbstractVideoSurface> m_surface;
    QMutex m_surfaceMutex;

    GPContext *m_context;
    Camera *m_camera;

    GPhotoCameraWorker *m_worker;
    QThread *m_workerThread;

    int m_lastImageCaptureId;
};

#endif // GPHOTOCAMERASESSION_H
