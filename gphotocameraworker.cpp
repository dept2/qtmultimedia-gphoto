#include "gphotocameraworker.h"

#include <QCameraImageCapture>

GPhotoCameraWorker::GPhotoCameraWorker(GPContext *context, Camera *camera, QObject *parent)
    : QObject(parent)
    , m_context(context)
    , m_camera(camera)
{
}

void GPhotoCameraWorker::capturePreview()
{
    emit previewCaptured(capturePreviewImage());
}

void GPhotoCameraWorker::capturePhoto(int id, const QString &fileName)
{
    QMutexLocker locker(&m_captureMutex);
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

QImage GPhotoCameraWorker::capturePreviewImage()
{
    QMutexLocker locker(&m_captureMutex);
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
        }

        gp_file_free(file);
    }

    return result;
}
