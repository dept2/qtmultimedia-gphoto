#ifndef GPHOTOVIDEOPROBECONTROL_H
#define GPHOTOVIDEOPROBECONTROL_H

#include <QMediaVideoProbeControl>

class GPhotoCameraSession;

class GPhotoVideoProbeControl : public QMediaVideoProbeControl
{
    Q_OBJECT
public:
    explicit GPhotoVideoProbeControl(GPhotoCameraSession *session, QObject *parent = 0);

private:
    GPhotoCameraSession *m_session;
};

#endif // GPHOTOVIDEOPROBECONTROL_H
