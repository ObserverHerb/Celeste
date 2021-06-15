#include <QtTest/QtTest>
#include <QTimer>
#include <iostream>
#include "recognizers.h"
#include "subscribers.h"
#include "window.h"

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

class TestWindow : public Window
{
	Q_OBJECT
public:
	TestWindow() : Window(), validResponse(true), arrivalSound("/dev/null")
	{
		connect(this,&TestWindow::RequestEphemeralPane,[this](AudioAnnouncePane *pane) {
			connect(pane,&AudioAnnouncePane::Finished,this,&TestWindow::GreenLight);
			connect(pane,&AudioAnnouncePane::DurationAvailable,[this](qint64 duration) {
				QWARN(QString::number(duration).toLatin1());
				QSignalSpy windowSpy(this,&TestWindow::GreenLight);
				QVERIFY(windowSpy.wait(duration));
			});
			QSignalSpy paneSpy(pane,&AudioAnnouncePane::DurationAvailable);
			QVERIFY(paneSpy.wait(5000));
		});
	}

	void TestDataAvailable()
	{
		DataAvailable();
	}

	void BreakResponses() { validResponse=false; }

	void SetArrivalSound(const QString& path) { arrivalSound=path; }

	TestEventSubscriber* EventSubscriber() const
	{
		return testEventSubscriber;
	}

protected:
	bool validResponse;
	QString arrivalSound;
	TestEventSubscriber *testEventSubscriber;

	QByteArray ReadFromSocket() const override
	{
		return validResponse ? ircSocket->readAll() : QByteArray("Things");
	}

	class EventSubscriber* CreateEventSubscriber(const QString &administratorID) override
	{
		testEventSubscriber=new TestEventSubscriber(administratorID,this);
		return testEventSubscriber;
	}

	const QString& ArrivalSound() const override
	{
		return arrivalSound;
	}

signals:
	void GreenLight();

protected slots:
	void JoinStream() override
	{
		emit GreenLight();
	}
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
	TestWindow testWindow;
	QDir dataPath;
private slots:
	void initTestCase()
	{
		testWindow.show();
	}

	void fileRecognizerTest()
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
	}

	void arrivalTest()
	{
		testWindow.AnnounceArrival(Viewer("Invalid Viewer")); // FIXME: This is expected to fail, but QSignalSpy doesn't realize that

		QDir path(dataPath);
		QVERIFY(path.cd("audio"));
		FileRecognizer recognizer(path.absolutePath());
		testWindow.SetArrivalSound(recognizer.Random());
		testWindow.AnnounceArrival(Viewer("Valid Viewer A"));
		testWindow.SetArrivalSound(recognizer.Random());
		testWindow.AnnounceArrival(Viewer("Valid Viewer B"));
	}

	void verifyConnected()
	{
		QSignalSpy windowSpy(&testWindow,&TestWindow::GreenLight);
		QVERIFY(windowSpy.wait(TimeConvert::Interval(testWindow.JoinDelay() + std::chrono::seconds(1))));
	}

	void goodSubscriptionDataTest()
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
		testWindow.BreakResponses();
		testWindow.TestDataAvailable();
		QVERIFY(true);

		QSignalSpy windowSpy(&testWindow,&TestWindow::GreenLight);
		QVERIFY(windowSpy.wait(30000));
	}
};

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cout << "ERROR: Please specify path to test data." << std::endl;
		return 1;
	}

	argc=1;
	QApplication app(argc,argv);

	Testing testing;
	testing.DataPath(argv[1]);

	return QTest::qExec(&testing,argc,argv);
}
#include "testing.moc"
