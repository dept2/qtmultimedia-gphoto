#include "gphotocamerasession.h"
#include "gphotocameraworker.h"

#include <QVideoSurfaceFormat>
#include <QCameraImageCapture>
#include <QThread>
#include <QStandardPaths>
#include <QFile>

GPhotoCameraSession::GPhotoCameraSession(QObject *parent)
    : QObject(parent)
    , m_state(QCamera::UnloadedState)
    , m_status(QCamera::UnloadedStatus)
    , m_captureMode(QCamera::CaptureStillImage)
    , m_captureDestination(QCameraImageCapture::CaptureToBuffer | QCameraImageCapture::CaptureToFile)
    , m_surface(0)
    , m_camera(0)
    , m_worker(0)
    , m_workerThread(new QThread(this))
    , m_lastImageCaptureId(0)
{
    // Create gphoto camera context
    m_context = gp_context_new();
    if (!m_context)
        m_status = QCamera::UnavailableStatus;
}

GPhotoCameraSession::~GPhotoCameraSession()
{
    if (m_status == QCamera::ActiveStatus)
        stopViewFinder();
    closeCamera();
}

QCamera::State GPhotoCameraSession::state() const
{
    return m_state;
}

void GPhotoCameraSession::setState(QCamera::State state)
{
    if (m_state == state)
        return;

    const QCamera::State previousState = m_state;

    if (previousState == QCamera::UnloadedState) {
        if (state == QCamera::LoadedState) {
            if (openCamera()) {
                m_state = state;
            }
        } else if (state == QCamera::ActiveState) {
            if (openCamera()) {
                startViewFinder();
                m_state = state;
            }
        }
    } else if (previousState == QCamera::LoadedState) {
        if (state == QCamera::UnloadedState) {
            closeCamera();
            m_state = state;
        } else if (state == QCamera::ActiveState) {
            startViewFinder();
            m_state = state;
        }
    } else if (previousState == QCamera::ActiveState) {
        if (state == QCamera::UnloadedState) {
            stopViewFinder();
            closeCamera();
            m_state = state;
        } else if (state == QCamera::LoadedState) {
            stopViewFinder();
            m_state = state;
        }
    }

    if (m_state != previousState)
        emit stateChanged(m_state);
}


QCamera::Status GPhotoCameraSession::status() const
{
    return m_status;
}

bool GPhotoCameraSession::isCaptureModeSupported(QCamera::CaptureModes mode) const
{
    if (mode == QCamera::CaptureViewfinder)
        return true;
    else if (mode == QCamera::CaptureStillImage)
        return true;

    return false;
}

QCamera::CaptureModes GPhotoCameraSession::captureMode() const
{
    return m_captureMode;
}

void GPhotoCameraSession::setCaptureMode(QCamera::CaptureModes captureMode)
{
    if (m_captureMode == captureMode)
        return;

    m_captureMode = captureMode;
    emit captureModeChanged(m_captureMode);
}

bool GPhotoCameraSession::isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations /*destination*/) const
{
    return true;
}

QCameraImageCapture::CaptureDestinations GPhotoCameraSession::captureDestination() const
{
    return m_captureDestination;
}

void GPhotoCameraSession::setCaptureDestination(QCameraImageCapture::CaptureDestinations destination)
{
    if (m_captureDestination != destination) {
        m_captureDestination = destination;
        emit captureDestinationChanged(destination);
    }
}

bool GPhotoCameraSession::isReadyForCapture() const
{
    if (m_captureMode & QCamera::CaptureStillImage)
        return (m_status == QCamera::ActiveStatus || m_status == QCamera::LoadedStatus);

    return false;
}

int GPhotoCameraSession::capture(const QString &fileName)
{
    m_lastImageCaptureId++;

    if (!isReadyForCapture()) {
        emit imageCaptureError(m_lastImageCaptureId, QCameraImageCapture::NotReadyError, tr("Camera not ready"));
        return m_lastImageCaptureId;
    }

    QMetaObject::invokeMethod(m_worker, "capturePhoto", Qt::QueuedConnection, Q_ARG(int, m_lastImageCaptureId),
                              Q_ARG(QString, fileName));
    return m_lastImageCaptureId;
}

QAbstractVideoSurface *GPhotoCameraSession::surface() const
{
    return m_surface;
}

void GPhotoCameraSession::setSurface(QAbstractVideoSurface *surface)
{
    QMutexLocker locker(&m_surfaceMutex);

    if (m_surface == surface)
        return;

    m_surface = surface;
}

void GPhotoCameraSession::previewCaptured(const QImage &image)
{
    if (m_status != QCamera::ActiveStatus)
        return;

    QMutexLocker locker(&m_surfaceMutex);
    if (m_surface) {
        if (image.size() != m_surface->surfaceFormat().frameSize()) {
            m_surface->stop();
            m_surface->start(QVideoSurfaceFormat(image.size(), QVideoFrame::Format_RGB32));
        }

        QVideoFrame frame(image);
        m_surface->present(frame);
    }

    QMetaObject::invokeMethod(m_worker, "capturePreview", Qt::QueuedConnection);
}

void GPhotoCameraSession::imageDataCaptured(int id, const QByteArray &imageData, const QString &fileName)
{
    QImage image = QImage::fromData(imageData);
    {
        QSize previewSize = image.size();
        int downScaleSteps = 0;
        while (previewSize.width() > 800 && downScaleSteps < 8) {
            previewSize.rwidth() /= 2;
            previewSize.rheight() /= 2;
            downScaleSteps++;
        }

        const QImage snapPreview = image.scaled(previewSize);
        emit imageCaptured(id, snapPreview);
    }

    if (m_captureDestination & QCameraImageCapture::CaptureToBuffer) {
        QVideoFrame frame(image);
        emit imageAvailable(id, frame);
    }

    if (m_captureDestination & QCameraImageCapture::CaptureToFile) {
        QString actualFileName(fileName);
        if (actualFileName.isEmpty()) {
            QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
            if (dir.isEmpty()) {
                emit imageCaptureError(id, QCameraImageCapture::ResourceError,
                                       tr("Could not determine writable location for saving captured image"));
                return;
            }

            dir += "/DCIM%1.jpg";
            // Trying to find free filename
            for (int i = 0; i < 9999; ++i) {
                QString f = dir.arg(i, 4, 10, QChar('0'));
                if (!QFile(f).exists()) {
                    actualFileName = f;
                    break;
                }
            }

            if (actualFileName.isEmpty()) {
                emit imageCaptureError(id, QCameraImageCapture::ResourceError,
                                       tr("Could not determine writable location for saving captured image"));
                return;
            }
        }

        QFile file(actualFileName);
        if (file.open(QFile::WriteOnly)) {
            if (file.write(imageData)) {
                emit imageSaved(id, actualFileName);
            } else {
                emit imageCaptureError(id, QCameraImageCapture::OutOfSpaceError, file.errorString());
            }
        } else {
            const QString errorMessage = tr("Could not open destination file:\n%1").arg(actualFileName);
            emit imageCaptureError(id, QCameraImageCapture::ResourceError, errorMessage);
        }
    }
}

bool GPhotoCameraSession::openCamera()
{
    // Camera is already open
    if (m_camera)
        return true;

    m_status = QCamera::LoadingStatus;
    emit statusChanged(m_status);
    emit readyForCaptureChanged(isReadyForCapture());

    // Create camera object
    int ret = gp_camera_new(&m_camera);
    if (ret != GP_OK) {
        m_camera = 0;
        m_status = QCamera::UnloadedStatus;
        emit statusChanged(m_status);
        emit readyForCaptureChanged(isReadyForCapture());

        qWarning() << "Unable to open camera";
        emit error(QCamera::CameraError, tr("Unable to open camera"));
        return false;
    }

    // Init camera object
    ret = gp_camera_init(m_camera, m_context);
    if (ret != GP_OK) {
        m_camera = 0;
        m_status = QCamera::UnloadedStatus;
        emit statusChanged(m_status);
        emit readyForCaptureChanged(isReadyForCapture());

        qWarning() << "Unable to open camera";
        emit error(QCamera::CameraError, tr("Unable to open camera"));
        return false;
    }

    m_worker = new GPhotoCameraWorker(m_context, m_camera);
    connect(m_worker, SIGNAL(previewCaptured(QImage)), SLOT(previewCaptured(QImage)));
    connect(m_worker, SIGNAL(imageCaptured(int,QByteArray,QString)), SLOT(imageDataCaptured(int,QByteArray,QString)));
    connect(m_worker, SIGNAL(imageCaptureError(int,int,QString)), SIGNAL(imageCaptureError(int,int,QString)));
    m_workerThread->start();
    m_worker->moveToThread(m_workerThread);

    m_status = QCamera::LoadedStatus;
    emit statusChanged(m_status);
    emit readyForCaptureChanged(isReadyForCapture());

    return true;
}

void GPhotoCameraSession::closeCamera()
{
    // Camera is already closed
    if (!m_camera)
        return;

    m_status = QCamera::UnloadingStatus;
    emit statusChanged(m_status);
    emit readyForCaptureChanged(isReadyForCapture());

    // Stop working thread
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_worker;

    // Close GPhoto camera session
    int ret = gp_camera_exit(m_camera, m_context);
    if (ret != GP_OK) {
        m_status = QCamera::LoadedStatus;
        emit statusChanged(m_status);
        emit readyForCaptureChanged(isReadyForCapture());

        qWarning() << "Unable to close camera";
        emit error(QCamera::CameraError, tr("Unable to close camera"));
        return;
    }

    gp_camera_free(m_camera);
    m_camera = 0;
    m_status = QCamera::UnloadedStatus;
    emit statusChanged(m_status);
    emit readyForCaptureChanged(isReadyForCapture());
}

bool GPhotoCameraSession::startViewFinder()
{
    m_status = QCamera::StartingStatus;
    emit statusChanged(m_status);
    emit readyForCaptureChanged(isReadyForCapture());

    // Capture first preview frame to detect the viewfinder size
    m_surfaceMutex.lock();
    QImage previewFrame = m_worker->capturePreviewImage();
    if (m_surface) {
        // Decoding JPEG image from camera produces RGB32 image
        const bool ok = m_surface->start(QVideoSurfaceFormat(previewFrame.size(), QVideoFrame::Format_RGB32));
        if (!ok)
            qWarning() << "Unable to start viewfinder surface";
    }
    m_surfaceMutex.unlock();


    m_status = QCamera::ActiveStatus;
    emit statusChanged(m_status);
    emit readyForCaptureChanged(isReadyForCapture());

    // Show the first captured preview frame and start acquiring more of it
    previewCaptured(previewFrame);

    return true;
}

void GPhotoCameraSession::stopViewFinder()
{
    m_status = QCamera::StoppingStatus;
    emit statusChanged(m_status);

    m_surfaceMutex.lock();
    if (m_surface)
        m_surface->stop();
    m_surfaceMutex.unlock();

    m_status = QCamera::LoadedStatus;
    emit statusChanged(m_status);
}
