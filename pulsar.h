#include <QObject>
#include <QJsonArray>

inline const char *PULSAR_SOCKET_NAME="pulsar-obs";

class Pulsar : public QObject
{
	Q_OBJECT
public:
	bool LoadTriggers();
protected:
	std::unordered_map<QString,QJsonArray> triggers;
	std::unordered_map<QString,QString> commandCrossReference;
signals:
	void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("Pulsar"));
public slots:
	void Pulse(const QString &trigger,const QString &command);
	void Pulse(const QString &trigger);
};
