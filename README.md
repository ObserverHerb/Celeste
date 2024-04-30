# Celeste

[Celeste](https://twitch.hlmjr.com/celeste.html) is a Twitch bot written in C++/[Qt](https://www.qt.io/product/framework). It's purpose is to watch for commands that appear in chat and function as a widget in your OBS scene collection that displays reactions to those commands, while maintaining a minimal CPU/memory footprint. It is cross-platform and open source.

## Configuration

Most configuration can be done via the UI (via right click context menu), but the configuration files can also be edited manually. There are in either JSON or INI format.

### Locations

All of Celeste's configuration files can be found in the `EngineeringDeck` folder of your [QStandardPaths::writableLocation](https://doc.qt.io/qt-6/qstandardpaths.html#StandardLocation-enum) location. Visit the [Qt documentation](https://doc.qt.io/qt-6/qstandardpaths.html#StandardLocation-enum) for the location specific to your platform.

The main configuration files are `Celeste.conf` (or `.ini` if you're on Windows) for media paths and layout options, and `Private.conf` for secure options such as your auth token.

The following files also exist:

* `commands.json` - List of user-defined commands and the media locations they point to.
* `songs.json` - List of songs that make up the vibe playlist
* `logs` - Directory holding logs for troubleshooting

### Installing

An installer is available for Windows on the [releases page](https://github.com/EngineeringDeck/Celeste/releases). Debian and rpm packages (and likely a Gentoo ebuild) will be avilable for Linux soon.

### Contributing

Celeste can be built from source on Windows, Linux, and macOS. You will need, at a minimum, the following:

* cmake >= v3.28
* A build of the Qt Framework that contains these modules:
  * QtWidgets
  * QtNetwork
  * QtMqtt
  * QtMultimedia
  * QtMultimediaWidgets
  * QtWebSockets

To build the Pulsar plugin for [OBS Studio](https://obsproject.com), you will need the OBS source in a directory named `obs-source` under the root of Celeste's source directory.
