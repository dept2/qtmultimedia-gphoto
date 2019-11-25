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
    GPhotoCameraWorker(const CameraAbilities &abilities, const GPPortInfo &portInfo, QObject *parent = nullptr);
    ~GPhotoCameraWorker();

    GPhotoCameraWorker(GPhotoCameraWorker&&) = delete;
    GPhotoCameraWorker&operator=(GPhotoCameraWorker&&) = delete;

signals:
    void statusChanged(QCamera::Status);
    void error(int error, const QString &errorString);

    void previewCaptured(const QImage&);

    void imageCaptured(int id, const QByteArray &imageData, const QString &fileName);
    void imageCaptureError(int id, int error, const QString &errorString);

public slots:
    void openCamera();
    void closeCamera();
    void startViewFinder();
    void stopViewFinder();

    void capturePreview();
    void capturePhoto(int id, const QString &fileName);

    QVariant parameter(const QString &name);
    bool setParameter(const QString &name, const QVariant &value);

private:
    Q_DISABLE_COPY(GPhotoCameraWorker)

    const CameraAbilities m_abilities;
    GPPortInfo m_portInfo;
    GPContextPtr m_context;
    CameraPtr m_camera;
    CameraFilePtr m_file;
    int m_capturingFailCount = 0;
    QCamera::Status m_status = QCamera::UnloadedStatus;

    void openCameraErrorHandle(const QString &errorText);
    void logOption(const char* name);
    void waitForOperationCompleted();
    void setStatus(QCamera::Status status);
};

#endif // GPHOTOCAMERAWORKER_H
