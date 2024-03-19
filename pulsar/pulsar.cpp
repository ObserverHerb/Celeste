#include <obs-module.h>
#include <obs-source.h>
#include <obs-frontend-api.h>
#include <util/base.h>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMainWindow>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include "pulsar.h"

OBS_DECLARE_MODULE()

const char *SUBSYSTEM_NAME="[Pulsar]";
const char *JSON_KEY_SOURCE_NAME="name";
const char *JSON_KEY_SCENE_NAME="name";
const char *JSON_KEY_SOURCE_TRIGGER="trigger";
const char *JSON_KEY_SOURCE_POSITION="position";
const char *JSON_KEY_SOURCE_POSITION_X="x";
const char *JSON_KEY_SOURCE_POSITION_Y="y";

void Log(const std::string &message)
{
	blog(LOG_INFO,"%s%s%s",SUBSYSTEM_NAME," ",message.data());
}

void Warn(const std::string &message)
{
	blog(LOG_WARNING,"%s%s%s",SUBSYSTEM_NAME," ",message.data());
}

// reference counts are increased with each API call unless documentation explicitly states otherwise
// ex. obs_scene_from_source() states "Does not increase the reference"
using ScenePtr=std::unique_ptr<obs_scene_t,decltype(&obs_scene_release)>;
using SceneItemPtr=std::unique_ptr<obs_sceneitem_t,decltype(&obs_sceneitem_release)>;
using SourcePtr=std::unique_ptr<obs_source_t,decltype(&obs_source_release)>;
using SourceListPtr=std::unique_ptr<obs_frontend_source_list,decltype(&obs_frontend_source_list_free)>;

enum Triggers
{
	SWITCH_SCENE,
	ENABLE_SOURCE,
	DISABLE_SOURCE,
	MOVE_SOURCE
};
std::unordered_map<std::string,int> triggers={
	{"switch_scene",SWITCH_SCENE},
	{"enable_source",ENABLE_SOURCE},
	{"disable_source",DISABLE_SOURCE},
	{"move_source",MOVE_SOURCE}
};

QLocalServer *server=nullptr;
std::unordered_set<QLocalSocket*> sockets;
QCheckBox *enabled=nullptr;

obs_sceneitem_t* FindSource(const std::string &name)
{
	obs_scene_t *scene=obs_scene_from_source(SourcePtr(obs_frontend_get_current_scene(),&obs_source_release).get()); // passing NULL into obs_scene_from_source() does not crash
	if (!scene) throw std::runtime_error("Could not determine current scene");
	obs_sceneitem_t *source=obs_scene_find_source(scene,name.data()); // does not increment reference! (https://obsproject.com/forum/threads/quesiton-of-obs_scene_find_source.106113/)
	if (!source)
	{
		source=obs_scene_get_group(scene,name.data()); // might be a group of sources instead of a single source
		if (!source) throw std::runtime_error("Could not find source ("+name+") in scene.");
	}
	return source;
}

void MoveSource(const std::string &name,const QJsonObject &jsonObject)
{
	Log("Move Source: "+name);

	if (jsonObject.isEmpty()) throw std::runtime_error("Invalid tranformation information");
	vec2 pos {
		jsonObject.value(JSON_KEY_SOURCE_POSITION_X).toVariant().toFloat(),
		jsonObject.value(JSON_KEY_SOURCE_POSITION_Y).toVariant().toFloat()
	};

	obs_sceneitem_set_pos(FindSource(name),&pos);
}

void ShowSource(const std::string &name)
{
	Log("Show Source: "+name);
	obs_sceneitem_set_visible(FindSource(name),true);
}

void HideSource(const std::string &name)
{
	Log("Hide Source: "+name);
	obs_sceneitem_set_visible(FindSource(name),false);
}

obs_source_t* FindScene(const std::string &name)
{
	SourceListPtr scenes(new obs_frontend_source_list({0}),&obs_frontend_source_list_free);
	obs_frontend_get_scenes(scenes.get());
	for (size_t sceneIndex=0; sceneIndex < scenes->sources.num; sceneIndex++)
	{
		obs_source_t *candidate=scenes->sources.array[sceneIndex];
		if (std::string(obs_source_get_name(candidate)) == name) return obs_source_get_ref(candidate);
	}
	throw std::runtime_error("Scene ("+name+") could not be found");
}

void SwitchScene(const std::string &name)
{
	obs_frontend_set_current_scene(SourcePtr(FindScene(name),&obs_source_release).get());
}

void SceneSwitched(const std::string &name)
{
	QJsonObject object;
	object.insert(JSON_KEY_SCENE,QString::fromStdString(name));
	for (QLocalSocket *socket : sockets) std::to_string(socket->write(QJsonDocument(object).toJson(QJsonDocument::Compact)+'\n'));
	Log("Scene Changed: "+name);
}

void Connection()
{
	Log("Incoming connection established");

	QLocalSocket *socket=server->nextPendingConnection();
	sockets.insert(socket);
	socket->connect(socket,&QLocalSocket::readyRead,socket,[socket]() {
		try
		{
			// read json from socket
			QByteArray data=socket->readAll().trimmed();

			if (!enabled->isChecked()) return;

			// parse triggers out of json
			QJsonParseError jsonError;
			QJsonDocument jsonDocument=QJsonDocument::fromJson(data,&jsonError);
			if (jsonDocument.isNull()) throw std::runtime_error("Failed to parse JSON: "+jsonError.errorString().toStdString());
			const QJsonArray jsonArray=jsonDocument.array();
			for (const QJsonValue &jsonValue : jsonArray)
			{
				QJsonObject jsonObject=jsonValue.toObject();

				// look for trigger name and bail if we don't recognize it
				std::string name=jsonObject.value(JSON_KEY_SOURCE_TRIGGER).toString().toStdString();
				Log("Trigger: "+name);
				if (triggers.find(name) == triggers.end()) throw std::runtime_error("Unrecognized trigger received: "+name);

				switch (triggers.at(name))
				{
				case SWITCH_SCENE:
				{
					SwitchScene(jsonObject.value(JSON_KEY_SCENE_NAME).toString().toStdString());
					break;
				}
				case ENABLE_SOURCE:
					ShowSource(jsonObject.value(JSON_KEY_SOURCE_NAME).toString().toStdString());
					break;
				case DISABLE_SOURCE:
					HideSource(jsonObject.value(JSON_KEY_SOURCE_NAME).toString().toStdString());
					break;
				case MOVE_SOURCE:
					MoveSource(jsonObject.value(JSON_KEY_SOURCE_NAME).toString().toStdString(),jsonObject.value(JSON_KEY_SOURCE_POSITION).toObject());
					break;
				}
			}
		}

		catch (const std::runtime_error &exception)
		{
			Warn(exception.what());
		}
	});
	socket->connect(socket,&QLocalSocket::disconnected,socket,[socket]() {
		sockets.erase(socket);
		socket->deleteLater();
	});
}

void HandleEvent(obs_frontend_event event,void *data)
{
	switch (event)
	{
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		SceneSwitched({obs_source_get_name(SourcePtr(obs_frontend_get_current_scene(),&obs_source_release).get())});
		break;
	}
}

bool obs_module_load()
{
	obs_frontend_add_event_callback(HandleEvent,nullptr);

	server=new QLocalServer();
	server->setSocketOptions(QLocalServer::UserAccessOption);
	server->connect(server,&QLocalServer::newConnection,server,&Connection);

	if (!server->listen(PULSAR_SOCKET_NAME))
	{
		Warn("Failed to create Pulsar server.");
		return false;
	}

	QMainWindow *window=static_cast<QMainWindow*>(obs_frontend_get_main_window());
	QDockWidget *dock=new QDockWidget("Pulsar",window);
	QWidget *container=new QWidget(dock);
	QVBoxLayout *layout=new QVBoxLayout(container);
	enabled=new QCheckBox("Enabled",container);
	enabled->setChecked(true);
	layout->addWidget(enabled);
	container->setLayout(layout);
	dock->setWidget(container);
	dock->setObjectName("celeste_pulsar_controls");
	window->addDockWidget(Qt::BottomDockWidgetArea,dock);
	obs_frontend_add_dock(dock);

	Log("Loaded");
	return true;
}

void obs_module_unload()
{
	obs_frontend_remove_event_callback(HandleEvent,nullptr);
	server->close();
	server->deleteLater();
}
