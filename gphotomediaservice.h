#ifndef GPHOTOMEDIASERVICE_H
#define GPHOTOMEDIASERVICE_H

#include <memory>

#include <QMediaService>

class GPhotoCameraSession;
class GPhotoFactory;

class GPhotoMediaService final : public QMediaService
{
    Q_OBJECT
public:
    explicit GPhotoMediaService(GPhotoFactory *factory, QObject *parent = nullptr);
    ~GPhotoMediaService();

    GPhotoMediaService(GPhotoMediaService&&) = delete;
    GPhotoMediaService& operator=(GPhotoMediaService&&) = delete;

    QMediaControl* requestControl(const char *name) override;
    void releaseControl(QMediaControl *control) override;

private:
    Q_DISABLE_COPY(GPhotoMediaService)

    GPhotoFactory *const m_factory;
    std::unique_ptr<GPhotoCameraSession> m_session;
};

#endif // GPHOTOMEDIASERVICE_H
