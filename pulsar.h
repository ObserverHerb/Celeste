#include <QObject>
#include <QJsonArray>
#include <unordered_map>

inline const char *PULSAR_SOCKET_NAME="pulsar-obs";

class Pulsar : public QObject
{
	Q_OBJECT
public:
	bool LoadTriggers();
protected:
	std::unordered_map<QString,QJsonArray> triggers;
signals:
	void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("Pulsar"));
public slots:
	void Pulse(const QString &trigger);
};
