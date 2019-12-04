#ifndef GPHOTOEXPOSURECONTROL_H
#define GPHOTOEXPOSURECONTROL_H

#include <QCameraExposureControl>

class GPhotoCameraSession;

class GPhotoExposureControl final : public QCameraExposureControl
{
    Q_OBJECT
public:
    explicit GPhotoExposureControl(GPhotoCameraSession *session, QObject *parent = nullptr);
    ~GPhotoExposureControl() = default;

    GPhotoExposureControl(GPhotoExposureControl&&) = delete;
    GPhotoExposureControl& operator=(GPhotoExposureControl&&) = delete;

    QVariant actualValue(ExposureParameter parameter) const final;
    bool isParameterSupported(ExposureParameter parameter) const final;
    QVariant requestedValue(ExposureParameter parameter) const final;
    bool setValue(ExposureParameter parameter, const QVariant &value) final;
    QVariantList supportedParameterRange(ExposureParameter parameter, bool *continuous) const final;

private slots:
    void stateChanged(QCamera::State);

private:
    Q_DISABLE_COPY(GPhotoExposureControl)

    GPhotoCameraSession *const m_session;
    QMap<QCameraExposureControl::ExposureParameter, QVariant> m_requestedValues;

    QCamera::State m_state;
};

#endif // GPHOTOEXPOSURECONTROL_H
