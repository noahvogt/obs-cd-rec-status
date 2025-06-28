#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QDockWidget>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("cd-rec-status", "en-US")

class CDStatusDock : public QWidget {
public:
    CDStatusDock(QWidget *parent = nullptr) : QWidget(parent), socket(new QWebSocket()) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        statusLabel = new QLabel("Connecting to server...", this);
        layout->addWidget(statusLabel);
        setLayout(layout);

        connect(socket, &QWebSocket::connected, this, &CDStatusDock::onConnected);
        connect(socket, &QWebSocket::disconnected, this, &CDStatusDock::onDisconnected);
        connect(socket, &QWebSocket::textMessageReceived, this, &CDStatusDock::onMessageReceived);

        socket->open(QUrl(QStringLiteral("ws://localhost:8765")));
    }

    ~CDStatusDock() {
        socket->close();
        delete socket;
    }


private slots:
    void onConnected() {
        statusLabel->setText("Connected to status server");
        updateBackgroundColor(Qt::gray);  // assume no recording yet
    }

    void onDisconnected() {
        statusLabel->setText("Disconnected from server");
        updateBackgroundColor(Qt::red);
    }

    void onMessageReceived(const QString &message) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            statusLabel->setText("Invalid status message");
            updateBackgroundColor(Qt::red);
            return;
        }

        QJsonObject obj = doc.object();
        bool isRecording = obj["recording"].toBool();

	QString statusText;
	if (isRecording) {
		statusText =
			QString("Recording: active\nCD: %2\nTrack: %3\nElapsed CD Time: %4\nElapsed Track Time: %5")
				.arg(obj["cd"].toInt())
				.arg(obj["track"].toInt())
				.arg(obj["cd_time"].toString())
				.arg(obj["track_time"].toString());
	} else {
		statusText = QString("Recording is currently not active.");
	}

	statusLabel->setText(statusText);
        updateBackgroundColor(isRecording ? QColor("#228B22") : Qt::gray);
    }

private:
    void updateBackgroundColor(const QColor &bgColor) {
    this->setStyleSheet(QString(
        "background-color: %1;"
        "color: %2;"
    ).arg(bgColor.name())
     .arg(isColorDark(bgColor) ? "white" : "black"));
}

bool isColorDark(const QColor &color) {
    // Perceived brightness formula
    int brightness = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
    return brightness < 128;
}


private:
    QLabel *statusLabel;
    QWebSocket *socket;
};

static void *create_cd_status_dock(obs_source_t *) {
    return new CDStatusDock();
}

bool obs_module_load(void)
{
    QDockWidget *dock = new QDockWidget("CD Rec Status");
    dock->setObjectName("cd-rec-status-dock");
    dock->setWidget(new CDStatusDock());

    obs_frontend_add_custom_qdock("cd_status_dock", (void *)dock);

    blog(LOG_INFO, "CD Rec Status Dock loaded!");
    return true;
}

MODULE_EXPORT const char *obs_module_description(void)
{
    return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
    return obs_module_text("CD Rec Status");
}
