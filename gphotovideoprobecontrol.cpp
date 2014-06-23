#include "gphotovideoprobecontrol.h"
#include "gphotocamerasession.h"

GPhotoVideoProbeControl::GPhotoVideoProbeControl(GPhotoCameraSession *session, QObject *parent)
    : QMediaVideoProbeControl(parent)
    , m_session(session)
{
    connect(m_session, SIGNAL(videoFrameProbed(QVideoFrame)), SIGNAL(videoFrameProbed(QVideoFrame)));
}
