#include "gphotocameraworker.h"

#include <QCameraImageCapture>

GPhotoCameraWorker::GPhotoCameraWorker(QObject *parent)
    : QObject(parent)
    , m_context(0)
    , m_camera(0)
    , m_status(QCamera::UnloadedStatus)
{
    // Create gphoto camera context
    m_context = gp_context_new();
    if (!m_context)
        m_status = QCamera::UnavailableStatus;
}

GPhotoCameraWorker::~GPhotoCameraWorker()
{
    closeCamera();
}

void GPhotoCameraWorker::openCamera()
{
    // Camera is already open
    if (m_camera)
        return;

    m_status = QCamera::LoadingStatus;
    emit statusChanged(m_status);

    // Create camera object
    int ret = gp_camera_new(&m_camera);
    if (ret != GP_OK) {
        m_camera = 0;
        m_status = QCamera::UnavailableStatus;
        emit statusChanged(m_status);

        qWarning() << "Unable to open camera";
        emit error(QCamera::CameraError, tr("Unable to open camera"));
        return;
    }

    // Init camera object
    ret = gp_camera_init(m_camera, m_context);
    if (ret != GP_OK) {
        m_camera = 0;
        m_status = QCamera::UnavailableStatus;
        emit statusChanged(m_status);

        qWarning() << "Unable to open camera";
        emit error(QCamera::CameraError, tr("Unable to open camera"));
        return;
    }

    m_status = QCamera::LoadedStatus;
    emit statusChanged(m_status);
}

void GPhotoCameraWorker::closeCamera()
{
    // Camera is already closed
    if (!m_camera)
        return;

    m_status = QCamera::UnloadingStatus;
    emit statusChanged(m_status);

    // Close GPhoto camera session
    int ret = gp_camera_exit(m_camera, m_context);
    if (ret != GP_OK) {
        m_status = QCamera::LoadedStatus;
        emit statusChanged(m_status);

        qWarning() << "Unable to close camera";
        emit error(QCamera::CameraError, tr("Unable to close camera"));
        return;
    }

    gp_camera_free(m_camera);
    m_camera = 0;
    m_status = QCamera::UnloadedStatus;
    emit statusChanged(m_status);
}

void GPhotoCameraWorker::stopViewFinder()
{
    emit statusChanged(QCamera::StoppingStatus);

    m_status = QCamera::LoadedStatus;
    emit statusChanged(QCamera::LoadedStatus);
}

void GPhotoCameraWorker::capturePreview()
{
    openCamera();

    if (m_status != QCamera::ActiveStatus) {
        m_status = QCamera::StartingStatus;
        emit statusChanged(m_status);
    }

    QImage result;

    CameraFile* file;
    int ret = gp_file_new(&file);
    if (ret < GP_OK) {
        qCritical() << "Could not create file";
    } else {
        ret = gp_camera_capture_preview(m_camera, file, m_context);

        if (ret < GP_OK) {
            qWarning() << "Failed retrieving preview";
        } else {
            const char* data;
            unsigned long int size = 0;

            gp_file_get_data_and_size(file, &data, &size);
            result.loadFromData(QByteArray(data, size));

            if (m_status != QCamera::ActiveStatus) {
                m_status = QCamera::ActiveStatus;
                emit statusChanged(m_status);
            }

            emit previewCaptured(result);
        }
        gp_file_free(file);
    }
}

void GPhotoCameraWorker::capturePhoto(int id, const QString &fileName)
{
    QByteArray result;

    // Capture the frame from camera
    CameraFilePath filePath;
    int ret = gp_camera_capture(m_camera, GP_CAPTURE_IMAGE, &filePath, m_context);

    if (ret < GP_OK) {
        qWarning() << "Failed to capture frame:" << ret;
        emit imageCaptureError(id, QCameraImageCapture::ResourceError, "Failed to capture frame");
    } else {
        qDebug() << "Captured frame:" << filePath.folder << filePath.name;

        // Download the file
        CameraFile* file;
        ret = gp_file_new(&file);
        ret = gp_camera_file_get(m_camera, filePath.folder, filePath.name, GP_FILE_TYPE_NORMAL, file, m_context);

        if (ret < GP_OK) {
            qWarning() << "Failed to get file from camera:" << ret;
            emit imageCaptureError(id, QCameraImageCapture::ResourceError, "Failed to download file from camera");
        } else {
            const char* data;
            unsigned long int size = 0;

            gp_file_get_data_and_size(file, &data, &size);
            result = QByteArray(data, size);
            emit imageCaptured(id, result, fileName);
        }

        gp_file_free(file);

        while(1) {
            CameraEventType type;
            void* data;
            ret = gp_camera_wait_for_event(m_camera, 100, &type, &data, m_context);
            if(type == GP_EVENT_TIMEOUT) {
                break;
            }
            else if (type == GP_EVENT_CAPTURE_COMPLETE) {
//                qDebug("Capture completed\n");
            }
            else if (type != GP_EVENT_UNKNOWN) {
                qWarning("Unexpected event received from camera: %d\n", (int)type);
            }
        }
    }
}
