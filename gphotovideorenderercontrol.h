#ifndef GPHOTOVIDEORENDERERCONTROL_H
#define GPHOTOVIDEORENDERERCONTROL_H

#include <QVideoRendererControl>

class GPhotoCameraSession;

class GPhotoVideoRendererControl final : public QVideoRendererControl
{
    Q_OBJECT
public:
    explicit GPhotoVideoRendererControl(GPhotoCameraSession *session, QObject *parent = nullptr);

    GPhotoVideoRendererControl(GPhotoVideoRendererControl&&) = delete;
    GPhotoVideoRendererControl& operator=(GPhotoVideoRendererControl&&) = delete;
    ~GPhotoVideoRendererControl() = default;

    QAbstractVideoSurface* surface() const override;
    void setSurface(QAbstractVideoSurface *surface) override;

private:
    Q_DISABLE_COPY(GPhotoVideoRendererControl)

    GPhotoCameraSession *const m_session;
};

#endif // GPHOTOVIDEORENDERERCONTROL_H
