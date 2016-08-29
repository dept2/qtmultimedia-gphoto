#ifndef GPHOTOVIDEORENDERERCONTROL_H
#define GPHOTOVIDEORENDERERCONTROL_H

#include <QVideoRendererControl>

class GPhotoCameraSession;

class GPhotoVideoRendererControl : public QVideoRendererControl
{
    Q_OBJECT
public:
    explicit GPhotoVideoRendererControl(GPhotoCameraSession *session, QObject *parent = 0);

    QAbstractVideoSurface* surface() const Q_DECL_OVERRIDE;
    void setSurface(QAbstractVideoSurface *surface) Q_DECL_OVERRIDE;

private:
    GPhotoCameraSession *m_session;
};

#endif // GPHOTOVIDEORENDERERCONTROL_H
