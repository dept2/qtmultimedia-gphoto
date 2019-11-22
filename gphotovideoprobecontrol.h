#ifndef GPHOTOVIDEOPROBECONTROL_H
#define GPHOTOVIDEOPROBECONTROL_H

#include <QMediaVideoProbeControl>

class GPhotoCameraSession;

class GPhotoVideoProbeControl final : public QMediaVideoProbeControl
{
    Q_OBJECT
public:
    explicit GPhotoVideoProbeControl(GPhotoCameraSession *session, QObject *parent = nullptr);

    GPhotoVideoProbeControl(GPhotoVideoProbeControl&&) = delete;
    GPhotoVideoProbeControl& operator=(GPhotoVideoProbeControl&&) = delete;
    ~GPhotoVideoProbeControl() = default;

private:
    Q_DISABLE_COPY(GPhotoVideoProbeControl)

    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOVIDEOPROBECONTROL_H
