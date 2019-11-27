#ifndef GPHOTOCAMERAWORKER_H
#define GPHOTOCAMERAWORKER_H

#include <QObject>
#include <gphoto2/gphoto2-camera.h>
#include "gphotocamerasession.h"
#include "gphotofactory.h"

using CameraFilePtr = std::unique_ptr<CameraFile, int (*)(CameraFile*)>;
using CameraPtr = std::unique_ptr<Camera, int (*)(Camera*)>;
using GPContextPtr = std::unique_ptr<GPContext, void (*)(GPContext*)>;

class GPhotoCameraWorker final : public QObject
{
    Q_OBJECT
public:
    enum class MirrorPosition {
        Up,
        Down
    };

    GPhotoCameraWorker(const CameraAbilities &abilities, const GPPortInfo &portInfo, QObject *parent = nullptr);
    ~GPhotoCameraWorker();

    GPhotoCameraWorker(GPhotoCameraWorker&&) = delete;
    GPhotoCameraWorker&operator=(GPhotoCameraWorker&&) = delete;

    Q_INVOKABLE void setState(QCamera::State state);
    Q_INVOKABLE void setCaptureMode(QCamera::CaptureModes captureMode);
    Q_INVOKABLE void capturePreview();
    Q_INVOKABLE void capturePhoto(int id, const QString &fileName);
    Q_INVOKABLE QVariant parameter(const QString &name);
    Q_INVOKABLE bool setParameter(const QString &name, const QVariant &value);

signals:
    void stateChanged(QCamera::State);
    void statusChanged(QCamera::Status);
    void captureModeChanged(QCamera::CaptureModes);
    void readyForCaptureChanged(bool);
    void error(int error, const QString &errorString);
    void previewCaptured(const QImage&);
    void imageCaptured(int id, const QByteArray &imageData, const QString &fileName);
    void imageCaptureError(int id, int error, const QString &errorString);

private:
    Q_DISABLE_COPY(GPhotoCameraWorker)

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

    const CameraAbilities m_abilities;
    GPPortInfo m_portInfo;
    GPContextPtr m_context;
    CameraPtr m_camera;
    CameraFilePtr m_file;
    int m_capturingFailCount = 0;
    QCamera::State m_state = QCamera::UnloadedState;
    QCamera::Status m_status = QCamera::UnloadedStatus;
    QCamera::CaptureModes m_captureMode = QCamera::CaptureStillImage;
};

#endif // GPHOTOCAMERAWORKER_H
