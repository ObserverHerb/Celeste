#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QLocalServer>
#include <QLocalSocket>
#include <unordered_map>
#include <iostream>

OBS_DECLARE_MODULE()

void HandleEvent(obs_frontend_event event,void *data)
{
	switch (event)
	{
		case OBS_FRONTEND_EVENT_SCENE_CHANGED:
			std::cout << "Scene Changed: " << obs_source_get_name(obs_frontend_get_current_scene()) << std::endl;
			break;
	}
}

enum Triggers
{
	SWITCH_SCENE,
	ENABLE_SOURCE,
	DISABLE_SOURCE
};

QLocalServer *server;
std::unordered_map<QByteArray,int> triggers;

bool obs_module_load()
{
	obs_frontend_add_event_callback(HandleEvent,nullptr);

	triggers.insert({"switch_scene",SWITCH_SCENE});
	triggers.insert({"enable_source",ENABLE_SOURCE});
	triggers.insert({"disable_source",DISABLE_SOURCE});

	server=new QLocalServer();
	server->connect(server,&QLocalServer::newConnection,[]() {
		std::cout << "Connection established!" << std::endl;
		QLocalSocket *socket=server->nextPendingConnection();
		socket->connect(socket,&QLocalSocket::readyRead,[socket]() {
			QList<QByteArray> data=socket->readAll().trimmed().split('|');
			QByteArray command=data.front();
			QByteArray message=data.back();
			std::cout << "Data received: " << command.toStdString() << std::endl;
			if (triggers.find(command) == triggers.end()) return;
			switch (triggers.at(command))
			{
			case SWITCH_SCENE:
			{
				std::cout << "Switch Scene" << std::endl;
				struct obs_frontend_source_list scenes={0};
				obs_frontend_get_scenes(&scenes);
				for (size_t scene=0; scene < scenes.sources.num; scene++)
				{
					obs_source_t *details=scenes.sources.array[scene];
					const char *name=obs_source_get_name(details);
					std::cout << "Testing name " << name << " vs. " << message.data() << std::endl;
					if (QString(name) == QString(message))
					{
						std::cout << "Setting scene...";
						obs_frontend_set_current_scene(details);
					}
				}

				break;
			}
			case ENABLE_SOURCE:
				std::cout << "Enable Source" << std::endl;
				break;
			case DISABLE_SOURCE:
				std::cout << "Disable Source" << std::endl;
				break;
			}
		});
	});
	if (!server->listen("pulsar-obs")) std::cout << "Failed to creatte Pulsar server." << std::endl;

	std::cout << "Pulsar loaded!" << std::endl;

	return true;
}

void obs_module_unload()
{
	delete server;
}
