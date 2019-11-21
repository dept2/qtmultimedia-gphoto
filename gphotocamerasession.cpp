#include "gphotocamerasession.h"
#include "gphotocameraworker.h"
#include "gphotofactory.h"

#include <QVideoSurfaceFormat>
#include <QCameraImageCapture>
#include <QThread>
#include <QStandardPaths>
#include <QFile>

GPhotoCameraSession::GPhotoCameraSession(GPhotoFactory *factory, QObject *parent)
    : QObject(parent)
    , m_factory(factory)
    , m_workerThread(new QThread(this))
{
    m_workerThread->start();
}

GPhotoCameraSession::~GPhotoCameraSession()
{
    for (auto worker : m_workers.values()) {
        // Disconnect status change signals on app shutdown not to trigger connected (and possibly also
        // dismantling) calling code
        disconnect(worker, nullptr, this, nullptr);
        QMetaObject::invokeMethod(worker, "closeCamera", Qt::BlockingQueuedConnection);
    }

    // Stop working thread
    m_workerThread->quit();
    m_workerThread->wait();
    qDeleteAll(m_workers);
}

QCamera::State GPhotoCameraSession::state() const
{
    return m_state;
}

void GPhotoCameraSession::setState(QCamera::State state)
{
    if (m_setStateRequired) {
        if (state == QCamera::UnloadedState) {
            QMetaObject::invokeMethod(m_currentWorker, "closeCamera", Qt::QueuedConnection);
        } else if (state == QCamera::LoadedState) {
            QMetaObject::invokeMethod(m_currentWorker, "openCamera", Qt::QueuedConnection);
        } else if (state == QCamera::ActiveState) {
            QMetaObject::invokeMethod(m_currentWorker, "startViewFinder", Qt::QueuedConnection);
        }

        m_setStateRequired = false;
        return;
    }

    if (m_state == state)
        return;

    const QCamera::State previousState = m_state;

    if (previousState == QCamera::UnloadedState) {
        if (state == QCamera::LoadedState) {
            QMetaObject::invokeMethod(m_currentWorker, "openCamera", Qt::QueuedConnection);
        } else if (state == QCamera::ActiveState) {
            QMetaObject::invokeMethod(m_currentWorker, "startViewFinder", Qt::QueuedConnection);
        }
    } else if (previousState == QCamera::LoadedState) {
        if (state == QCamera::UnloadedState) {
            QMetaObject::invokeMethod(m_currentWorker, "closeCamera", Qt::QueuedConnection);
        } else if (state == QCamera::ActiveState) {
            QMetaObject::invokeMethod(m_currentWorker, "startViewFinder", Qt::QueuedConnection);
        }
    } else if (previousState == QCamera::ActiveState) {
        if (state == QCamera::UnloadedState) {
            QMetaObject::invokeMethod(m_currentWorker, "closeCamera", Qt::QueuedConnection);
        } else if (state == QCamera::LoadedState) {
            QMetaObject::invokeMethod(m_currentWorker, "stopViewFinder", Qt::QueuedConnection);
        }
    }
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

bool GPhotoCameraSession::isCaptureDestinationSupported(CaptureDestinations /*destination*/) const
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

    QMetaObject::invokeMethod(m_currentWorker, "capturePhoto", Qt::QueuedConnection,
                              Q_ARG(int, m_lastImageCaptureId), Q_ARG(QString, fileName));
    return m_lastImageCaptureId;
}

QAbstractVideoSurface* GPhotoCameraSession::surface() const
{
    return m_surface;
}

void GPhotoCameraSession::setSurface(QAbstractVideoSurface *surface)
{
    if (m_surface == surface)
        return;

    m_surface = surface;
}

QVariant GPhotoCameraSession::parameter(const QString &name)
{
    QVariant result;
    QMetaObject::invokeMethod(m_currentWorker, "parameter", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, result), Q_ARG(QString, name));
    return result;
}

bool GPhotoCameraSession::setParameter(const QString &name, const QVariant &value)
{
    bool result;
    QMetaObject::invokeMethod(m_currentWorker, "setParameter", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result), Q_ARG(QString, name), Q_ARG(QVariant, value));
    return result;
}

void GPhotoCameraSession::setCamera(int cameraIndex)
{
    GPhotoCameraWorker *worker = getWorker(cameraIndex);

    if (m_currentWorker != worker) {
        disconnect(worker, nullptr, this, nullptr);

        connect(worker, SIGNAL(statusChanged(QCamera::Status)), SLOT(workerStatusChanged(QCamera::Status)));
        connect(worker, SIGNAL(error(int,QString)), SIGNAL(error(int,QString)));
        connect(worker, SIGNAL(previewCaptured(QImage)), SLOT(previewCaptured(QImage)));
        connect(worker, SIGNAL(imageCaptured(int,QByteArray,QString)), SLOT(imageDataCaptured(int,QByteArray,QString)));
        connect(worker, SIGNAL(imageCaptureError(int,int,QString)), SIGNAL(imageCaptureError(int,int,QString)));

        m_currentWorker = worker;
        m_setStateRequired = true;
    }
}

void GPhotoCameraSession::previewCaptured(const QImage &image)
{
    if (m_status == QCamera::ActiveStatus && m_surface && !image.isNull()) {
        if (m_surface->isActive() && image.size() != m_surface->surfaceFormat().frameSize())
            m_surface->stop();

        if (!m_surface->isActive())
            m_surface->start(QVideoSurfaceFormat(image.size(), QVideoFrame::Format_RGB32));

        QVideoFrame frame(image);
        m_surface->present(frame);
        emit videoFrameProbed(frame);
    }

    if (m_status == QCamera::ActiveStatus)
        QMetaObject::invokeMethod(m_currentWorker, "capturePreview", Qt::QueuedConnection);
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

        const QImage &snapPreview = image.scaled(previewSize);
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
            const QString &errorMessage = tr("Could not open destination file:\n%1").arg(actualFileName);
            emit imageCaptureError(id, QCameraImageCapture::ResourceError, errorMessage);
        }
    }
}

void GPhotoCameraSession::workerStatusChanged(QCamera::Status status)
{
    if (status != m_status) {
        m_status = status;

        if (status == QCamera::LoadedStatus) {
            m_state = QCamera::LoadedState;
            emit stateChanged(m_state);
            emit readyForCaptureChanged(isReadyForCapture());
        } else if (status == QCamera::ActiveStatus) {
            m_state = QCamera::ActiveState;
            emit stateChanged(m_state);
            emit readyForCaptureChanged(isReadyForCapture());
        } else if (status == QCamera::UnloadedStatus || status == QCamera::UnavailableStatus) {
            m_state = QCamera::UnloadedState;
            emit stateChanged(m_state);
            emit readyForCaptureChanged(isReadyForCapture());
        }

        emit statusChanged(status);
    }
}

GPhotoCameraWorker* GPhotoCameraSession::getWorker(int cameraIndex)
{
    if (!m_workers.contains(cameraIndex)) {
        Q_ASSERT(m_factory);

        bool ok = false;
        const CameraAbilities &abilities = m_factory->cameraAbilities(cameraIndex, &ok);
        if (!ok) return nullptr;

        ok = false;
        const PortInfo &portInfo = m_factory->portInfo(cameraIndex, &ok);
        if (!ok) return nullptr;

        GPhotoCameraWorker *worker = new GPhotoCameraWorker(abilities, portInfo);
        worker->moveToThread(m_workerThread);

        m_workers.insert(cameraIndex, worker);
        return worker;
    }

    return m_workers[cameraIndex];
}
