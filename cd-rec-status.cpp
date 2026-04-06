#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QDockWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QColor>
#include <thread>
#include <mutex>
#include <chrono>

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

// Use the asio_client config from asio_no_tls_client.hpp
typedef websocketpp::client<websocketpp::config::asio_client> client;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("cd-rec-status", "en-US")

class CDStatusDock : public QWidget {
public:
    CDStatusDock(QWidget *parent = nullptr) : QWidget(parent), retryDotCount(0), m_connected(false), m_stop_ws(false) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        statusLabel = new QLabel("Connecting to websocket...", this);
        layout->addWidget(statusLabel);
        setLayout(layout);

        retryDotCount = 0;

        // Reconnect every 3s if disconnected
        reconnectTimer = new QTimer(this);
        reconnectTimer->setInterval(3000);
        connect(reconnectTimer, &QTimer::timeout, this, &CDStatusDock::tryReconnect);

        // Animate retrying text every 500ms
        retryAnimationTimer = new QTimer(this);
        retryAnimationTimer->setInterval(500);
        connect(retryAnimationTimer, &QTimer::timeout, this, &CDStatusDock::updateRetryingLabel);

        m_ws_client.clear_access_channels(websocketpp::log::alevel::all);
        m_ws_client.init_asio();

        m_ws_client.set_open_handler([this](websocketpp::connection_hdl) {
            QMetaObject::invokeMethod(this, [this](){ onConnected(); }, Qt::QueuedConnection);
        });

        m_ws_client.set_close_handler([this](websocketpp::connection_hdl) {
            QMetaObject::invokeMethod(this, [this](){ onDisconnected(); }, Qt::QueuedConnection);
        });

        m_ws_client.set_fail_handler([this](websocketpp::connection_hdl) {
            QMetaObject::invokeMethod(this, [this](){ onDisconnected(); }, Qt::QueuedConnection);
        });

        m_ws_client.set_message_handler([this](websocketpp::connection_hdl, client::message_ptr msg) {
            QString message = QString::fromStdString(msg->get_payload());
            QMetaObject::invokeMethod(this, [this, message](){ onMessageReceived(message); }, Qt::QueuedConnection);
        });

        m_ws_thread = std::thread([this]() {
            while (!m_stop_ws) {
                try {
                    m_ws_client.run();
                } catch (const std::exception & e) {
                    blog(LOG_ERROR, "[cd-rec-status] WebSocket client run error: %s", e.what());
                }
                
                if (m_stop_ws) break;
                
                m_ws_client.reset();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });

        tryReconnect();
    }

    ~CDStatusDock() {
        reconnectTimer->stop();
        retryAnimationTimer->stop();
        m_stop_ws = true;
        m_ws_client.stop();
        if (m_ws_thread.joinable()) {
            m_ws_thread.join();
        }
    }

private:
    void onConnected() {
        m_connected = true;
        reconnectTimer->stop();
        retryAnimationTimer->stop();
        statusLabel->setText("Connected to websocket");
        updateBackgroundColor(Qt::gray);
    }

    void onDisconnected() {
        m_connected = false;
        retryDotCount = 0;
        reconnectTimer->start();
        retryAnimationTimer->start();
        updateRetryingLabel();
        updateBackgroundColor(Qt::red);
    }

    void tryReconnect() {
        if (m_connected)
            return;

        websocketpp::lib::error_code ec;
        client::connection_ptr con = m_ws_client.get_connection("ws://localhost:8765", ec);
        if (ec) {
            onDisconnected();
            return;
        }

        m_ws_client.connect(con);
        updateRetryingLabel();
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
                QString("Recording: active\nCD: %1\nTrack: %2\nElapsed CD Time: %3\nElapsed Track Time: %4")
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

    void updateBackgroundColor(const QColor &bgColor) {
        this->setStyleSheet(QString(
            "background-color: %1;"
            "color: %2;"
        ).arg(bgColor.name())
         .arg(isColorDark(bgColor) ? "white" : "black"));
    }

    bool isColorDark(const QColor &color) {
        int brightness = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
        return brightness < 128;
    }

    void updateRetryingLabel() {
        retryDotCount = (retryDotCount % 3) + 1;
        QString dots = QString(".").repeated(retryDotCount);
        statusLabel->setText(QString("Disconnected from websocket\nRetrying%1").arg(dots));
    }

private:
    QLabel *statusLabel;
    int retryDotCount;

    QTimer *reconnectTimer;
    QTimer *retryAnimationTimer;

    client m_ws_client;
    std::thread m_ws_thread;
    bool m_connected;
    bool m_stop_ws;
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
