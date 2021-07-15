#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QLocalSocket>
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

bool obs_module_load(void)
{
	obs_frontend_add_event_callback(HandleEvent,nullptr);

	return true;
}
