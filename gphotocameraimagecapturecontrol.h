#ifndef GPHOTOCAMERAIMAGECAPTURECONTROL_H
#define GPHOTOCAMERAIMAGECAPTURECONTROL_H

#include <QCameraImageCaptureControl>

class GPhotoCameraSession;

class GPhotoCameraImageCaptureControl : public QCameraImageCaptureControl
{
    Q_OBJECT
public:
    explicit GPhotoCameraImageCaptureControl(GPhotoCameraSession *session, QObject *parent = 0);

    QCameraImageCapture::DriveMode driveMode() const Q_DECL_OVERRIDE;
    void setDriveMode(QCameraImageCapture::DriveMode driveMode) Q_DECL_OVERRIDE;

    bool isReadyForCapture() const Q_DECL_OVERRIDE;

    int capture(const QString &fileName) Q_DECL_OVERRIDE;
    void cancelCapture() Q_DECL_OVERRIDE;

private:
    GPhotoCameraSession *m_session;
};

#endif // GPHOTOCAMERAIMAGECAPTURECONTROL_H
