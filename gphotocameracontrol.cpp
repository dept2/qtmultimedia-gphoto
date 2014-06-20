#include "gphotocameracontrol.h"
#include "gphotocamerasession.h"

GPhotoCameraControl::GPhotoCameraControl(GPhotoCameraSession *session, QObject *parent)
    : QCameraControl(parent)
    , m_session(session)
{
    connect(m_session, SIGNAL(statusChanged(QCamera::Status)), SIGNAL(statusChanged(QCamera::Status)));
    connect(m_session, SIGNAL(stateChanged(QCamera::State)), SIGNAL(stateChanged(QCamera::State)));
    connect(m_session, SIGNAL(error(int,QString)), SIGNAL(error(int,QString)));

    connect(m_session, SIGNAL(captureModeChanged(QCamera::CaptureModes)), SIGNAL(captureModeChanged(QCamera::CaptureModes)));
}

QCamera::State GPhotoCameraControl::state() const
{
    return m_session->state();
}

void GPhotoCameraControl::setState(QCamera::State state)
{
    return m_session->setState(state);
}

QCamera::Status GPhotoCameraControl::status() const
{
    return m_session->status();
}

bool GPhotoCameraControl::isCaptureModeSupported(QCamera::CaptureModes mode) const
{
    return m_session->isCaptureModeSupported(mode);
}

QCamera::CaptureModes GPhotoCameraControl::captureMode() const
{
    return m_session->captureMode();
}

void GPhotoCameraControl::setCaptureMode(QCamera::CaptureModes captureMode)
{
    m_session->setCaptureMode(captureMode);
}

bool GPhotoCameraControl::canChangeProperty(QCameraControl::PropertyChangeType /*changeType*/, QCamera::Status /*status*/) const
{
    return false;
}
