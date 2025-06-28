#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QLabel>
#include <QFile>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QDockWidget>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("show_cd_rec_status", "en-US")

class CDStatusDock : public QWidget {
public:
    CDStatusDock(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        statusLabel = new QLabel("Loading...", this);
        layout->addWidget(statusLabel);
        setLayout(layout);

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &CDStatusDock::updateStatus);
        timer->start(1000);
    }

private:
    QLabel *statusLabel;

    void updateStatus() {
        QFile f("/tmp/cd_status.txt");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString text = f.readAll();
            statusLabel->setText(text);
        } else {
            statusLabel->setText("Status file not found");
        }
    }
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
