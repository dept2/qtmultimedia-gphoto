#include "gphotocamerasession.h"
#include "gphotocameraworker.h"
#include "gphotofactory.h"

#include <QCameraImageCapture>
#include <QDebug>
#include <QFile>
#include <QStandardPaths>
#include <QThread>
#include <QVideoSurfaceFormat>

GPhotoCameraSession::GPhotoCameraSession(GPhotoFactory *factory, QObject *parent)
    : QObject(parent)
    , m_factory(factory)
    , m_workerThread(new QThread)
{
    m_workerThread->start();
}

GPhotoCameraSession::~GPhotoCameraSession()
{
    // Disconnect status change signals on app shutdown not to trigger connected (and possibly also
    // dismantling) calling code
    disconnect(m_currentWorker.get(), nullptr, this, nullptr);
    // Stop working thread
    m_workerThread->quit();
    m_workerThread->wait();
}

QCamera::State GPhotoCameraSession::state() const
{
    return m_state;
}

void GPhotoCameraSession::setState(QCamera::State state)
{
    QMetaObject::invokeMethod(m_currentWorker.get(), "setState",
                              Qt::QueuedConnection, Q_ARG(QCamera::State, state));
}

QCamera::Status GPhotoCameraSession::status() const
{
    return m_status;
}

bool GPhotoCameraSession::isCaptureModeSupported(QCamera::CaptureModes mode) const
{
    return (mode == QCamera::CaptureViewfinder || mode == QCamera::CaptureStillImage);
}

QCamera::CaptureModes GPhotoCameraSession::captureMode() const
{
    return m_captureMode;
}

void GPhotoCameraSession::setCaptureMode(QCamera::CaptureModes captureMode)
{
    QMetaObject::invokeMethod(m_currentWorker.get(), "setCaptureMode",
                              Qt::QueuedConnection, Q_ARG(QCamera::CaptureModes, captureMode));
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
    return m_readyForCapture;
}

int GPhotoCameraSession::capture(const QString &fileName)
{
    ++m_lastImageCaptureId;

    QMetaObject::invokeMethod(m_currentWorker.get(), "capturePhoto", Qt::QueuedConnection,
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
    QMetaObject::invokeMethod(m_currentWorker.get(), "parameter", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, result), Q_ARG(QString, name));
    return result;
}

bool GPhotoCameraSession::setParameter(const QString &name, const QVariant &value)
{
    bool result;
    QMetaObject::invokeMethod(m_currentWorker.get(), "setParameter", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result), Q_ARG(QString, name), Q_ARG(QVariant, value));
    return result;
}

void GPhotoCameraSession::setCamera(int cameraIndex)
{
    auto worker = getWorker(cameraIndex);

    if (!worker) {
        qWarning() << "Failed to set camera with index" << cameraIndex;
        return;
    }

    if (m_currentWorker != worker) {
        disconnect(worker.get(), nullptr, this, nullptr);

        using Worker = GPhotoCameraWorker;
        using Session = GPhotoCameraSession;

        connect(worker.get(), &Worker::stateChanged, this, &Session::onWorkerStateChanged);
        connect(worker.get(), &Worker::statusChanged, this, &Session::onWorkerStatusChanged);
        connect(worker.get(), &Worker::captureModeChanged, this, &Session::onWorkerReadyForCaptureChanged);
        connect(worker.get(), &Worker::readyForCaptureChanged, this, &Session::onWorkerReadyForCaptureChanged);
        connect(worker.get(), &Worker::error, this, &Session::error);
        connect(worker.get(), &Worker::previewCaptured, this, &Session::onPreviewCaptured);
        connect(worker.get(), &Worker::imageCaptured, this, &Session::onImageDataCaptured);
        connect(worker.get(), &Worker::imageCaptureError, this, &Session::imageCaptureError);

        m_currentWorker = std::move(worker);

        setState(m_state);
        setCaptureMode(m_captureMode);
    }
}

void GPhotoCameraSession::onPreviewCaptured(const QImage &image)
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
        QMetaObject::invokeMethod(m_currentWorker.get(), "capturePreview", Qt::QueuedConnection);
}

void GPhotoCameraSession::onImageDataCaptured(int id, const QByteArray &imageData, const QString &fileName)
{
    auto image = QImage::fromData(imageData);
    {
        auto previewSize = image.size();
        auto downScaleSteps = 0;
        while (previewSize.width() > 800 && downScaleSteps < 8) {
            previewSize.rwidth() /= 2;
            previewSize.rheight() /= 2;
            ++downScaleSteps;
        }

        const auto &snapPreview = image.scaled(previewSize);
        emit imageCaptured(id, snapPreview);
    }

    if (m_captureDestination & QCameraImageCapture::CaptureToBuffer) {
        QVideoFrame frame(image);
        emit imageAvailable(id, frame);
    }

    if (m_captureDestination & QCameraImageCapture::CaptureToFile) {
        QString actualFileName(fileName);
        if (actualFileName.isEmpty()) {
            auto dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
            if (dir.isEmpty()) {
                emit imageCaptureError(id, QCameraImageCapture::ResourceError,
                                       tr("Could not determine writable location for saving captured image"));
                return;
            }

            dir += "/DCIM%1.jpg";
            // Trying to find free filename
            for (int i = 0; i < 9999; ++i) {
                const auto &f = dir.arg(i, 4, 10, QChar('0'));
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

void GPhotoCameraSession::onWorkerStateChanged(QCamera::State state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}

void GPhotoCameraSession::onWorkerStatusChanged(QCamera::Status status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged(status);
    }
}

void GPhotoCameraSession::onCaptureModeChanged(QCamera::CaptureMode captureMode)
{
    if (m_captureMode != captureMode) {
        m_captureMode = captureMode;
        emit captureModeChanged(captureMode);
    }
}

void GPhotoCameraSession::onWorkerReadyForCaptureChanged(bool readyForCapture)
{
    if (m_readyForCapture != readyForCapture) {
        m_readyForCapture = readyForCapture;
        emit readyForCaptureChanged(readyForCapture);
    }
}

std::shared_ptr<GPhotoCameraWorker> GPhotoCameraSession::getWorker(int cameraIndex)
{
    if (!m_workers.contains(cameraIndex)) {
        Q_ASSERT(m_factory);

        auto ok = false;

        const auto &abilities = m_factory->cameraAbilities(cameraIndex, &ok);
        if (!ok) {
            qWarning() << "Unable to get abilities for camera with index" << cameraIndex;
            return nullptr;
        }

        const auto &portInfo = m_factory->portInfo(cameraIndex, &ok);
        if (!ok) {
            qWarning() << "Unable to get port info for camera with index" << cameraIndex;
            return nullptr;
        }

        auto worker = std::make_shared<GPhotoCameraWorker>(abilities, portInfo);
        worker->moveToThread(m_workerThread.get());

        m_workers.insert(cameraIndex, worker);
        return worker;
    }

    return m_workers.value(cameraIndex);
}
