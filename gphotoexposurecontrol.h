#ifndef GPHOTOEXPOSURECONTROL_H
#define GPHOTOEXPOSURECONTROL_H

#include <QCameraExposureControl>

class GPhotoCameraSession;

class GPhotoExposureControl : public QCameraExposureControl
{
    Q_OBJECT
public:
    explicit GPhotoExposureControl(GPhotoCameraSession *session, QObject *parent = 0);

    QVariant actualValue(ExposureParameter parameter) const;
    bool isParameterSupported(ExposureParameter parameter) const;
    QVariant requestedValue(ExposureParameter parameter) const;
    bool setValue(ExposureParameter parameter, const QVariant &value);
    QVariantList supportedParameterRange(ExposureParameter parameter, bool *continuous) const;

private slots:
    void stateChanged(QCamera::State);

private:
    GPhotoCameraSession *m_session;
    QMap<QCameraExposureControl::ExposureParameter, QVariant> m_requestedValues;

    QCamera::State m_state;
};

#endif // GPHOTOEXPOSURECONTROL_H
