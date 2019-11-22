#ifndef GPHOTOEXPOSURECONTROL_H
#define GPHOTOEXPOSURECONTROL_H

#include <QCameraExposureControl>

class GPhotoCameraSession;

class GPhotoExposureControl final : public QCameraExposureControl
{
    Q_OBJECT
public:
    explicit GPhotoExposureControl(GPhotoCameraSession *session, QObject *parent = nullptr);

    GPhotoExposureControl(GPhotoExposureControl&&) = delete;
    GPhotoExposureControl& operator=(GPhotoExposureControl&&) = delete;
    ~GPhotoExposureControl() = default;

    QVariant actualValue(ExposureParameter parameter) const override;
    bool isParameterSupported(ExposureParameter parameter) const override;
    QVariant requestedValue(ExposureParameter parameter) const override;
    bool setValue(ExposureParameter parameter, const QVariant &value) override;
    QVariantList supportedParameterRange(ExposureParameter parameter, bool *continuous) const override;

private slots:
    void stateChanged(QCamera::State);

private:
    Q_DISABLE_COPY(GPhotoExposureControl)

    GPhotoCameraSession *const m_session;
    QMap<QCameraExposureControl::ExposureParameter, QVariant> m_requestedValues;

    QCamera::State m_state;
};

#endif // GPHOTOEXPOSURECONTROL_H
