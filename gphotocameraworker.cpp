#include "gphotocameraworker.h"

#include <QCameraImageCapture>

namespace {
  const int capturingFailLimit = 10;
}

QDebug operator<<(QDebug dbg, const CameraWidgetType &t)
{
    switch (t) {
    case GP_WIDGET_WINDOW:
        dbg.nospace() << "GP_WIDGET_WINDOW";
        break;
    case GP_WIDGET_SECTION:
        dbg.nospace() << "GP_WIDGET_SECTION";
        break;
    case GP_WIDGET_TEXT:
        dbg.nospace() << "GP_WIDGET_TEXT";
        break;
    case GP_WIDGET_RANGE:
        dbg.nospace() << "GP_WIDGET_RANGE";
        break;
    case GP_WIDGET_TOGGLE:
        dbg.nospace() << "GP_WIDGET_TOGGLE";
        break;
    case GP_WIDGET_RADIO:
        dbg.nospace() << "GP_WIDGET_RADIO";
        break;
    case GP_WIDGET_MENU:
        dbg.nospace() << "GP_WIDGET_MENU";
        break;
    case GP_WIDGET_BUTTON:
        dbg.nospace() << "GP_WIDGET_BUTTON";
        break;
    case GP_WIDGET_DATE:
        dbg.nospace() << "GP_WIDGET_DATE";
        break;
    }

    return dbg.space();
}

GPhotoCameraWorker::GPhotoCameraWorker(const CameraAbilities &abilities, const PortInfo &portInfo, QObject *parent)
    : QObject(parent)
    , m_abilities(abilities)
    , m_portInfo(portInfo)
    , m_context(gp_context_new())
    , m_camera(0)
    , m_file(0)
    , m_capturingFailCount(0)
    , m_status(QCamera::UnloadedStatus)
{
    if (!m_context)
        m_status = QCamera::UnavailableStatus;
}

GPhotoCameraWorker::~GPhotoCameraWorker()
{
    closeCamera();
    gp_port_info_list_free(m_portInfo.portInfoList);
    gp_context_unref(m_context);
}

void GPhotoCameraWorker::openCamera()
{
    // Camera is already open
    if (m_camera)
        return;

    setStatus(QCamera::LoadingStatus);

    // Create camera object
    int ret = gp_camera_new(&m_camera);
    if (ret != GP_OK) {
        openCameraErrorHandle("Unable to open camera");
        return;
    }

    ret = gp_camera_set_abilities(m_camera, m_abilities);
    if (ret < GP_OK) {
        openCameraErrorHandle("Unable to set abilities for camera");
        return;
    }

    ret = gp_camera_set_port_info(m_camera, m_portInfo.portInfo);
    if (ret < GP_OK) {
        openCameraErrorHandle("Unable to set port info for camera");
        return;
    }

    ret = gp_file_new(&m_file);
    if (ret < GP_OK) {
        openCameraErrorHandle("Could not create capture file");
        return;
    }

    if (parameter("viewfinder").isValid()) {
        if (!setParameter("viewfinder", true))
            qWarning() << "Failed to flap up camera mirror";
    }

    m_capturingFailCount = 0;
    setStatus(QCamera::LoadedStatus);
}

void GPhotoCameraWorker::closeCamera()
{
    // Camera is already closed
    if (!m_camera)
        return;

    if (m_status == QCamera::ActiveStatus)
        stopViewFinder();

    setStatus(QCamera::UnloadingStatus);

    // Pull down mirror on Canon camera when closing the camera: a workaround for gphoto failing to close the mirror on
    // older cameras when calling gp_camera_exit
    setParameter("viewfinder", false);

    // Close GPhoto camera session
    int ret = gp_camera_exit(m_camera, m_context);
    if (ret != GP_OK) {
        qWarning() << "Failed to close camera";
        emit error(QCamera::CameraError, tr("Failed to close camera"));
    }

    gp_file_free(m_file);
    m_file = 0;
    gp_camera_free(m_camera);
    m_camera = 0;
    setStatus(QCamera::UnloadedStatus);
}

void GPhotoCameraWorker::startViewFinder()
{
    openCamera();

    if (m_status == QCamera::ActiveStatus)
        return;

    setStatus(QCamera::StartingStatus);
    setStatus(QCamera::ActiveStatus);

    capturePreview();
}

void GPhotoCameraWorker::stopViewFinder()
{
    setStatus(QCamera::StoppingStatus);
    setStatus(QCamera::LoadedStatus);
}

void GPhotoCameraWorker::capturePreview()
{
    gp_file_clean(m_file);

    if (m_status != QCamera::ActiveStatus)
        return;

    int ret = gp_camera_capture_preview(m_camera, m_file, m_context);
    if (GP_OK == ret) {
        const char* data;
        unsigned long int size = 0;
        ret = gp_file_get_data_and_size(m_file, &data, &size);
        if (GP_OK == ret) {
            m_capturingFailCount = 0;
            const QImage &result = QImage::fromData(QByteArray(data, int(size)));
            emit previewCaptured(result);
            return;
        }
    }

    qWarning() << "Failed retrieving preview" << ret;
    ++m_capturingFailCount;

    if (m_capturingFailCount >= capturingFailLimit)
    {
      qWarning() << "Closing camera because of capturing fail";
      emit error(QCamera::CameraError, tr("Unable to capture frame"));
      setStatus(QCamera::UnloadingStatus);
      closeCamera();
    }
}

void GPhotoCameraWorker::capturePhoto(int id, const QString &fileName)
{
    // Focusing
    if (parameter("viewfinder").isValid()) {
        if (!setParameter("viewfinder", false))
            qWarning() << "Failed to flap down camera mirror";
    } else {
        if (parameter("autofocusdrive").isValid())
            setParameter("autofocusdrive", true);
    }

    // Capture the frame from camera
    CameraFilePath filePath;
    int ret = gp_camera_capture(m_camera, GP_CAPTURE_IMAGE, &filePath, m_context);

    if (ret < GP_OK) {
        qWarning() << "Failed to capture frame:" << ret;
        emit imageCaptureError(id, QCameraImageCapture::ResourceError, "Failed to capture frame");
    } else {
        qDebug() << "Captured frame:" << filePath.folder << filePath.name;

        // Download the file
        CameraFile* file;
        ret = gp_file_new(&file);
        ret = gp_camera_file_get(m_camera, filePath.folder, filePath.name, GP_FILE_TYPE_NORMAL, file, m_context);

        if (ret < GP_OK) {
            qWarning() << "Failed to get file from camera:" << ret;
            emit imageCaptureError(id, QCameraImageCapture::ResourceError, "Failed to download file from camera");
        } else {
            const char* data;
            unsigned long int size = 0;

            ret = gp_file_get_data_and_size(file, &data, &size);
            if (ret < GP_OK) {
                qWarning() << "Failed to get file data and size from camera:" << ret;
                emit imageCaptureError(id, QCameraImageCapture::ResourceError, "Failed to download file from camera");
            } else {
                emit imageCaptured(id, QByteArray(data, int(size)), fileName);
            }
        }

        gp_file_free(file);
        waitForOperationCompleted();
    }

    if (parameter("viewfinder").isValid()) {
        if (!setParameter("viewfinder", true))
            qWarning() << "Failed to flap up camera mirror";
    }
}

QVariant GPhotoCameraWorker::parameter(const QString &name)
{
    CameraWidget *root;
    int ret = gp_camera_get_config(m_camera, &root, m_context);
    if (ret < GP_OK) {
        qWarning() << "Unable to get root option from gphoto";
        return QVariant();
    }

    CameraWidget *option;
    ret = gp_widget_get_child_by_name(root, qPrintable(name), &option);
    if (ret < GP_OK) {
        qWarning() << "Unable to get config widget from gphoto";
        return QVariant();
    }

    CameraWidgetType type;
    ret = gp_widget_get_type(option, &type);
    if (ret < GP_OK) {
        qWarning() << "Unable to get config widget type from gphoto";
        return QVariant();
    }

    if (type == GP_WIDGET_RADIO) {
        char *value;
        ret = gp_widget_get_value(option, &value);
        if (ret < GP_OK) {
            qWarning() << "Unable to get value for option" << qPrintable(name) << "from gphoto";
            return QVariant();
        } else {
            return QString::fromLocal8Bit(value);
        }
    } else if (type == GP_WIDGET_TOGGLE) {
        int value;
        ret = gp_widget_get_value(option, &value);
        if (ret < GP_OK) {
            qWarning() << "Unable to get value for option" << qPrintable(name) << "from gphoto";
            return QVariant();
        } else {
            return value == 0 ? false : true;
        }
    } else {
        qWarning() << "Options of type" << type << "are currently not supported";
    }

    return QVariant();
}

bool GPhotoCameraWorker::setParameter(const QString &name, const QVariant &value)
{
    CameraWidget *root;
    int ret = gp_camera_get_config(m_camera, &root, m_context);
    if (ret < GP_OK) {
        qWarning() << "Unable to get root option from gphoto";
        return false;
    }

    // Get widget pointer
    CameraWidget *option;
    ret = gp_widget_get_child_by_name(root, qPrintable(name), &option);
    if (ret < GP_OK) {
        qWarning() << "Unable to get option" << qPrintable(name) << "from gphoto";
        return false;
    }

    // Get option type
    CameraWidgetType type;
    ret = gp_widget_get_type(option, &type);
    if (ret < GP_OK) {
        qWarning() << "Unable to get option type from gphoto";
        gp_widget_free(option);
        return false;
    }

    if (type == GP_WIDGET_RADIO) {
        if (value.type() == QVariant::String) {
            // String, need no conversion
            ret = gp_widget_set_value(option, qPrintable(value.toString()));

            if (ret < GP_OK) {
                qWarning() << "Failed to set value" << value << "to" << name << "option:" << ret;
                return false;
            }

            ret = gp_camera_set_config(m_camera, root, m_context);

            if (ret < GP_OK) {
                qWarning() << "Failed to set config to camera";
                return false;
            }

            waitForOperationCompleted();
            return true;
        } else if (value.type() == QVariant::Double) {
            // Trying to find nearest possible value (with the distance of 0.1) and set it to property
            double v = value.toDouble();

            int count = gp_widget_count_choices(option);
            for (int i = 0; i < count; ++i) {
                const char* choice;
                gp_widget_get_choice(option, i, &choice);

                // We use a workaround for flawed russian i18n of gphoto2 strings
                bool ok;
                double choiceValue = QString::fromLocal8Bit(choice).replace(',', '.').toDouble(&ok);
                if (!ok) {
                    qDebug() << "Failed to convert value" << choice << "to double";
                    continue;
                }

                if (qAbs(choiceValue - v) < 0.1) {
                    ret = gp_widget_set_value(option, choice);
                    if (ret < GP_OK) {
                        qWarning() << "Failed to set value" << choice << "to" << name << "option:" << ret;
                        return false;
                    }

                    ret = gp_camera_set_config(m_camera, root, m_context);
                    if (ret < GP_OK) {
                        qWarning() << "Failed to set config to camera";
                        return false;
                    }

                    waitForOperationCompleted();
                    return true;
                }
            }

            qWarning() << "Can't find value matching to" << v << "for option" << name;
            return false;
        } else if (value.type() == QVariant::Int) {
            // Little hacks for 'ISO' option: if the value is -1, we pick the first non-integer value
            // we found and set it as a parameter
            int v = value.toInt();


            int count = gp_widget_count_choices(option);
            for (int i = 0; i < count; ++i) {
                const char* choice;
                gp_widget_get_choice(option, i, &choice);

                bool ok;
                int choiceValue = QString::fromLocal8Bit(choice).toInt(&ok);

                if ((ok && choiceValue == v) || (!ok && v == -1)) {
                    ret = gp_widget_set_value(option, choice);
                    if (ret < GP_OK) {
                        qWarning() << "Failed to set value" << choice << "to" << name << "option:" << ret;
                        return false;
                    }

                    ret = gp_camera_set_config(m_camera, root, m_context);
                    if (ret < GP_OK) {
                        qWarning() << "Failed to set config to camera";
                        return false;
                    }

                    waitForOperationCompleted();
                    return true;
                }
            }

            qWarning() << "Can't find value matching to" << v << "for option" << name;
            return false;
        } else {
            qWarning() << "Failed to set value" << value << "to" << name << "option. Type" << value.type()
                       << "is not supported";
            gp_widget_free(option);
            return false;
        }
    } else if (type == GP_WIDGET_TOGGLE) {
        int v = 0;
        if (value.canConvert<int>()) {
            v = value.toInt();
        } else {
            qWarning() << "Failed to set value" << value << "to" << name << "option. Type" << value.type()
                       << "is not supported";
            gp_widget_free(option);
            return false;
        }

        ret = gp_widget_set_value(option, &v);
        if (ret < GP_OK) {
          qWarning() << "Failed to set value" << v << "to" << name << "option:" << ret;
          return false;
        }

        ret = gp_camera_set_config(m_camera, root, m_context);
        if (ret < GP_OK) {
          qWarning() << "Failed to set config to camera";
          return false;
        }

        waitForOperationCompleted();
        return true;
    } else {
        qWarning() << "Options of type" << type << "are currently not supported";
    }

    gp_widget_free(option);
    return false;
}

void GPhotoCameraWorker::openCameraErrorHandle(const QString& errorText)
{
    qWarning() << qPrintable(errorText);
    setStatus(QCamera::UnavailableStatus);
    emit error(QCamera::CameraError, tr("Unable to open camera"));
    gp_camera_free(m_camera);
    m_camera = 0;
}

void GPhotoCameraWorker::logOption(const char *name)
{
    CameraWidget *root;
    int ret = gp_camera_get_config(m_camera, &root, m_context);
    if (ret < GP_OK) {
        qWarning() << "Unable to get root option from gphoto";
        return;
    }

    CameraWidget *option;
    ret = gp_widget_get_child_by_name(root, name, &option);
    if (ret < GP_OK)
        qWarning() << "Unable to get config widget from gphoto";

    CameraWidgetType type;
    ret = gp_widget_get_type(option, &type);
    if (ret < GP_OK)
        qWarning() << "Unable to get config widget type from gphoto";

    char *value;
    ret = gp_widget_get_value(option, &value);

    qDebug() << "Option" << type << name << value;
    if (type == GP_WIDGET_RADIO) {
        int count = gp_widget_count_choices(option);
        qDebug() << "Choices count:" << count;

        for (int i = 0; i < count; ++i) {
            const char* choice;
            gp_widget_get_choice(option, i, &choice);
            qDebug() << "  value:" << choice;
        }
    }

    gp_widget_free(option);
}

void GPhotoCameraWorker::waitForOperationCompleted()
{
    CameraEventType type;
    void *data;
    int ret;

    do {
        ret = gp_camera_wait_for_event(m_camera, 10, &type, &data, m_context);
        free(data);
    } while ((ret == GP_OK) && (type != GP_EVENT_TIMEOUT));
}

void GPhotoCameraWorker::setStatus(QCamera::Status status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged(status);
    }
}
