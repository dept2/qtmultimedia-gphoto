#ifndef GPHOTOCAMERA_H
#define GPHOTOCAMERA_H

#include <memory>

#include <QCamera>
#include <QObject>

#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-file.h>
#include <gphoto2/gphoto2-port-info-list.h>

using CameraFilePtr = std::unique_ptr<CameraFile, int (*)(CameraFile*)>;
using CameraPtr = std::unique_ptr<Camera, int (*)(Camera*)>;

class GPhotoCamera final : public QObject
{
    Q_OBJECT
public:
    enum class MirrorPosition {
        Up,
        Down
    };

    GPhotoCamera(GPContext *context, const CameraAbilities &abilities,
                 const GPPortInfo &portInfo, QObject *parent = nullptr);
    ~GPhotoCamera();

    GPhotoCamera(GPhotoCamera&&) = delete;
    GPhotoCamera&operator=(GPhotoCamera&&) = delete;

    void setState(QCamera::State state);
    void setCaptureMode(QCamera::CaptureModes captureMode);
    void capturePhoto(int id, const QString &fileName);

    QVariant parameter(const QString &name);
    bool setParameter(const QString &name, const QVariant &value);

signals:
    void captureModeChanged(QCamera::CaptureModes);
    void error(int error, const QString &errorString);
    void imageCaptured(int id, const QByteArray &imageData, const QString &fileName);
    void imageCaptureError(int id, int error, const QString &errorString);
    void previewCaptured(const QImage &image);
    void readyForCaptureChanged(bool);
    void stateChanged(QCamera::State);
    void statusChanged(QCamera::Status);

private slots:
    void capturePreview();

private:
    Q_DISABLE_COPY(GPhotoCamera)

    void openCamera();
    void closeCamera();
    void startViewFinder();
    void stopViewFinder();
    void setMirrorPosition(MirrorPosition pos);
    bool isReadyForCapture() const;
    void logOption(const char* name);
    void openCameraErrorHandle(const QString &errorText);
    void setStatus(QCamera::Status status);
    void waitForOperationCompleted();

    GPContext *const m_context;
    CameraAbilities m_abilities;
    GPPortInfo m_portInfo;
    CameraPtr m_camera;
    CameraFilePtr m_file;
    QCamera::State m_state = QCamera::UnloadedState;
    QCamera::Status m_status = QCamera::UnloadedStatus;
    QCamera::CaptureModes m_captureMode = QCamera::CaptureStillImage;
    int m_capturingFailCount = 0;
};

#endif // GPHOTOCAMERA_H
