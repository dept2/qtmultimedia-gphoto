#include "gphotocamerasession.h"
#include "gphotoexposurecontrol.h"

namespace {
    constexpr auto exposureParameter = "exposurecompensation";
    constexpr auto isoParameter = "iso";
}

GPhotoExposureControl::GPhotoExposureControl(GPhotoCameraSession *session, QObject *parent)
    : QCameraExposureControl(parent)
    , m_session(session)
{
    m_state = m_session->state();

    using Session = GPhotoCameraSession;
    using Control = GPhotoExposureControl;

    connect(m_session, &Session::stateChanged, this, &Control::stateChanged);
}

QVariant GPhotoExposureControl::actualValue(QCameraExposureControl::ExposureParameter parameter) const
{
    if (m_state == QCamera::UnloadedState)
        return QVariant();

    if (parameter == ExposureCompensation) {
        const auto &value = m_session->parameter(QLatin1Literal(exposureParameter));

        // We use a workaround for flawed russian i18n of gphoto2 strings
        auto ok = false;
        auto result = value.toString().replace(',', '.').toDouble(&ok);
        return ok ? QVariant(result) : QVariant();
    }

    if (parameter == ISO) {
        auto ok = false;
        auto iso = m_session->parameter(QLatin1Literal(isoParameter)).toInt(&ok);
        // Invalid QVariant for Auto ISO
        return ok ? QVariant(iso) : QVariant();
    }

    return QVariant();
}

bool GPhotoExposureControl::isParameterSupported(QCameraExposureControl::ExposureParameter parameter) const
{
    switch (parameter) {
    case QCameraExposureControl::ISO:
        return true;
    case QCameraExposureControl::Aperture:
        return false;
    case QCameraExposureControl::ShutterSpeed:
        return false;
    case QCameraExposureControl::ExposureCompensation:
        return true;
    case QCameraExposureControl::FlashPower:
        return false;
    case QCameraExposureControl::TorchPower:
        return false;
    case QCameraExposureControl::FlashCompensation:
        return false;
    case QCameraExposureControl::SpotMeteringPoint:
        return false;
    case QCameraExposureControl::ExposureMode:
        return false;
    case QCameraExposureControl::MeteringMode:
        return false;
    case QCameraExposureControl::ExtendedExposureParameter:
        return false;
    }

    return false;
}

QVariant GPhotoExposureControl::requestedValue(QCameraExposureControl::ExposureParameter parameter) const
{
    qDebug("requestedValue %d", parameter);
    return m_requestedValues.value(parameter);
}

bool GPhotoExposureControl::setValue(QCameraExposureControl::ExposureParameter parameter, const QVariant &value)
{
    m_requestedValues[parameter] = value;
    emit requestedValueChanged(parameter);

    // Try to set parameters only on loaded camera
    if (m_state == QCamera::UnloadedState)
        return false;

    if (parameter == ExposureCompensation) {
        auto result = m_session->setParameter(QLatin1Literal(exposureParameter), value);
        if (result) {
            emit actualValueChanged(parameter);
            return true;
        }
    } else if (parameter == ISO) {
        QVariant v = value;
        if (!v.isValid())
            v = -1;
        auto result = m_session->setParameter(QLatin1Literal(isoParameter), v);
        if (result) {
            emit actualValueChanged(parameter);
            return true;
        }
    } else {
        qWarning() << "Currently unsupported parameter" << parameter << "change requested";
    }

    return false;
}

QVariantList GPhotoExposureControl::supportedParameterRange(QCameraExposureControl::ExposureParameter parameter, bool *continuous) const
{
    Q_UNUSED(continuous)

    qDebug("supportedParameterRange %d", parameter);
    return QVariantList();
}

void GPhotoExposureControl::stateChanged(QCamera::State state)
{
    if (m_state != state) {
        if (m_state == QCamera::UnloadedState && state == QCamera::LoadedState) {
            m_state = state;

            QMetaEnum parameter = metaObject()->enumerator(metaObject()->indexOfEnumerator("ExposureParameter"));
            for (auto i = 0; i < parameter.keyCount(); ++i) {
                auto p = ExposureParameter(parameter.value(i));

                if (isParameterSupported(p)) {
                    // Set all parameters requested on start to session object
                    if (m_requestedValues.contains(p))
                        setValue(p, m_requestedValues.value(p));
                    // or just notify frontend that it's allowed to get the parameter values from backend
                    else
                        emit actualValueChanged(p);
                }
            }
        } else {
            m_state = state;
        }
    }
}
