#include <QtTest/QtTest>
#include <QTimer>
#include <iostream>
#include "channel.h"
#include "recognizers.h"
#include "subscribers.h"
#include "window.h"

const std::chrono::seconds INTERVAL_NETWORK_TIMEOUT(30);

class TestEventSubscriber : public EventSubscriber
{
public:
	TestEventSubscriber(const QString &administratorID,QObject *parent=nullptr) : EventSubscriber(administratorID,parent) { }

	void TestDataAvailable()
	{
		DataAvailable();
	}

	void Data(const QByteArray &data) { this->data=data; }
protected:
	QByteArray data;

	QTcpSocket* SocketFromSignalSender() const override
	{
		return nullptr;
	}

	QByteArray ReadFromSocket(QTcpSocket *socket=nullptr) const override
	{
		return data;
	}

	bool WriteToSocket(const QByteArray &data,QTcpSocket *socket=nullptr) const override
	{
		Print(Console::GenerateMessage(SUBSYSTEM_NAME,"write",data));
		return true;
	}
};

class TestIRCSocket : public IRCSocket
{
	Q_OBJECT

public:
	void SetResponse(const QString &data)
	{
		response=data;
	}

	void ObtainResponse()
	{
		response.clear();
	}

	virtual QByteArray Read() override
	{
		return response.isEmpty() ? readAll() : StringConvert::ByteArray(response);
	}

protected:
	QString response;

};

class TestChannel : public Channel
{
	Q_OBJECT

public:
	TestChannel(TestIRCSocket *socket,QObject *parent) : Channel(socket,parent)
	{

	}

	void ForceDataAvailable()
	{
		DataAvailable();
	}
};

class TestWindow : public Window
{
	Q_OBJECT
public:
	TestWindow() : Window(), validResponse(true), arrivalSound("/dev/null")
	{
		connect(this,&TestWindow::RequestEphemeralPane,[this](EphemeralPane *pane) {
			connect(pane,&AudioAnnouncePane::Finished,this,&TestWindow::GreenLight);
			connect(dynamic_cast<AudioAnnouncePane*>(pane),&AudioAnnouncePane::DurationAvailable,[this](qint64 duration) {
				QWARN(QString::number(duration).toLatin1());
				QSignalSpy windowSpy(this,&TestWindow::GreenLight);
				QVERIFY(windowSpy.wait(duration));
			});
			QSignalSpy paneSpy(dynamic_cast<AudioAnnouncePane*>(pane),&AudioAnnouncePane::DurationAvailable);
			QVERIFY(paneSpy.wait(5000));
		});
	}

	void SetArrivalSound(const QString& path) { arrivalSound=path; }

	TestEventSubscriber* EventSubscriber() const
	{
		return testEventSubscriber;
	}

protected:
	bool validResponse;
	QString arrivalSound;
	TestEventSubscriber *testEventSubscriber;

	class EventSubscriber* CreateEventSubscriber(const QString &administratorID) override
	{
		testEventSubscriber=new TestEventSubscriber(administratorID,this);
		return testEventSubscriber;
	}

	const QString ArrivalSound() const override
	{
		return arrivalSound;
	}

signals:
	void GreenLight() const;
};

class Testing : public QObject
{
	Q_OBJECT

public:
	void DataPath(const QString &path)
	{
		dataPath.setPath(path);
	}

	QByteArray DataFromFile(const QString &filename)
	{
		QFile file(dataPath.filePath(filename));
		if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Failed to open data file");
		QByteArray data=file.readAll();
		file.close();
		return data;
	}
private:
	//TestWindow testWindow;
	QDir dataPath;
private slots:
	void Print(const QString &message)
	{
		qInfo() << message;
	}

	void initTestCase()
	{
		//testWindow.show();
	}

	/*void fileRecognizerTest()
	{
		QDir path(dataPath);
		QVERIFY(path.cd("recognize"));
		FileRecognizer recognizer(path.absolutePath());
		QCOMPARE(QFileInfo(recognizer.First()).fileName(),"first.txt");

		// FIXME: Make this a legit test rather than just prints
		QWARN(recognizer.Random().toLatin1());
		QWARN(recognizer.Random().toLatin1());
		QWARN(recognizer.Random().toLatin1());
		QWARN(recognizer.Random().toLatin1());
		QWARN(recognizer.Random().toLatin1());
	}*/

	void invalidUser()
	{
		UserRecognizer *invalidViewer=new UserRecognizer("Invalid Viewer");
		QSignalSpy recognizerSpy(invalidViewer,&UserRecognizer::Error);
		QVERIFY(recognizerSpy.wait(5000));
	}

	void validUser()
	{
		UserRecognizer *validViewer=new UserRecognizer("Valid Viewer");
		QSignalSpy recognizerSpy(validViewer,&UserRecognizer::Recognized);
		QVERIFY(recognizerSpy.wait(5000));
	}

	void arrivalTest()
	{
		/*testWindow.AnnounceArrival(std::make_shared<UserRecognizer>("Invalid Viewer")); // FIXME: This is expected to fail, but QSignalSpy doesn't realize that

		QDir path(dataPath);
		QVERIFY(path.cd("audio"));
		FileRecognizer recognizer(path.absolutePath());
		testWindow.SetArrivalSound(recognizer.Random());
		testWindow.AnnounceArrival(std::make_shared<UserRecognizer>("Valid Viewer A"));
		testWindow.SetArrivalSound(recognizer.Random());
		testWindow.AnnounceArrival(std::make_shared<UserRecognizer>("Valid Viewer B"));*/
	}

	/*void goodSubscriptionDataTest()
	{
		try
		{
			testWindow.EventSubscriber()->Data(DataFromFile("channel_redemption.txt"));
			testWindow.EventSubscriber()->TestDataAvailable();

			testWindow.EventSubscriber()->Data(DataFromFile("channel_follow.txt"));
			testWindow.EventSubscriber()->TestDataAvailable();

			testWindow.EventSubscriber()->Data(DataFromFile("channel_raid.txt"));
			testWindow.EventSubscriber()->TestDataAvailable();

			testWindow.EventSubscriber()->Data(DataFromFile("channel_subscription.txt"));
			testWindow.EventSubscriber()->TestDataAvailable();
		}

		catch (const std::runtime_error &exception)
		{
			std::cout << "Critical error: " << exception.what() << std::endl;
		}

		catch (...)
		{
			std::cout << "Unknown critical error occurred!" << std::endl;
		}
	}

	void badSubscriptionDataTest()
	{
		try
		{
		}

		catch (const std::runtime_error &exception)
		{
			std::cout << "Critical error: " << exception.what() << std::endl;
		}

		catch (...)
		{
			std::cout << "Unknown critical error occurred!" << std::endl;
		}
	}

	void multipleMessagesTest()
	{
		TestIRCSocket socket;
		TestChannel channel(&socket,nullptr);
		connect(&channel,&Channel::Print,this,&Testing::Print);
		channel.Connect();
		QSignalSpy channelSpy(&channel,QOverload<ChatMessageReceiver*>::of(&TestChannel::Joined));
		QVERIFY(channelSpy.wait(TimeConvert::Interval(channel.JoinDelay()+std::chrono::seconds(1))));
		QString data=DataFromFile("chat_message.txt");
		int size=data.size();
		ChatMessageReceiver *chatMessageReceiver=channelSpy.takeFirst().at(0).value<ChatMessageReceiver*>();
		connect(chatMessageReceiver,&ChatMessageReceiver::Alert,this,[](const QString &message) {
			qWarning() << message; // but we do want to see why processing failed for context
		});
		QSignalSpy messageSpy(chatMessageReceiver,&ChatMessageReceiver::Print);
		for (int count=0; count < size; count++)
		{
			QMetaObject::invokeMethod(this,[&socket,&channel,&data,&count]() {
				// trigger using event loop so it doesn't fire before we wait for a response
				socket.SetResponse(data.left(count)+":gauntlet!gauntlet@gauntlet.tmi.twitch.tv PRIVMSG gauntlet :Test\r\n");
				channel.ForceDataAvailable();
			},Qt::QueuedConnection);
			QVERIFY(messageSpy.wait(3000));
		}
		data+=data;
		for (int count=size*2; count >= size; count--)
		{
			QMetaObject::invokeMethod(chatMessageReceiver,[&socket,&channel,&data,&count]() {
				// trigger using event loop so it doesn't fire before we wait for a response
				socket.SetResponse(data.left(count));
				channel.ForceDataAvailable();
			},Qt::QueuedConnection);
			QVERIFY(messageSpy.wait(3000));
		}
		QVERIFY(true);
	}*/
};

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cout << "ERROR: Please specify path to test data." << std::endl;
		return 1;
	}

	argc=1;
	QApplication application(argc,argv);
	application.setOrganizationName(ORGANIZATION_NAME);
	application.setApplicationName(APPLICATION_NAME);

	Testing testing;
	testing.DataPath(argv[1]);

	return QTest::qExec(&testing,argc,argv);
}
#include "testing.moc"
