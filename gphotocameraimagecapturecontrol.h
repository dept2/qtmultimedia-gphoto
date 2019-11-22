#ifndef GPHOTOCAMERAIMAGECAPTURECONTROL_H
#define GPHOTOCAMERAIMAGECAPTURECONTROL_H

#include <QCameraImageCaptureControl>

class GPhotoCameraSession;

class GPhotoCameraImageCaptureControl final : public QCameraImageCaptureControl
{
    Q_OBJECT
public:
    explicit GPhotoCameraImageCaptureControl(GPhotoCameraSession *session, QObject *parent = nullptr);

    GPhotoCameraImageCaptureControl(GPhotoCameraImageCaptureControl&&) = delete;
    GPhotoCameraImageCaptureControl& operator=(GPhotoCameraImageCaptureControl&&) = delete;
    ~GPhotoCameraImageCaptureControl() = default;

    QCameraImageCapture::DriveMode driveMode() const override;
    void setDriveMode(QCameraImageCapture::DriveMode driveMode) override;

    bool isReadyForCapture() const override;

    int capture(const QString &fileName) override;
    void cancelCapture() override;

private:
    Q_DISABLE_COPY(GPhotoCameraImageCaptureControl)

    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOCAMERAIMAGECAPTURECONTROL_H
