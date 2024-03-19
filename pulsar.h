#include <QObject>
#include <QJsonArray>
#include <QLocalSocket>
#include <QTimer>
#include "settings.h"

inline const char *PULSAR_SOCKET_NAME="pulsar-obs";
inline const char *JSON_KEY_SCENE="scene";

class Pulsar : public QObject
{
	Q_OBJECT
public:
	Pulsar();
	bool LoadTriggers();
protected:
	std::unordered_map<QString,QJsonArray> triggers;
	std::unordered_map<QString,QSize> dimensions;
	QLocalSocket *socket;
	QTimer reconnectDelay;
	ApplicationSetting settingReconnectDelay;
	void Connect();
signals:
	void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("Pulsar"));
	void Dimensions(const QSize &dimensions);
public slots:
	void Pulse(const QString &trigger);
protected slots:
	void Error(QLocalSocket::LocalSocketError error);
	void Reconnect();
	void Data();
};
