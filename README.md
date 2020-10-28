# Celeste

Celeste is a Twitch chat bot written in C++/Qt. It's purpose is to watch for commands that appear in chat and function as a widget in your OBS scene collection that displays reactions to those commands. It is cross-platform and open source.

## Configuration and Data Locations

### Configuration

The main configuration file is `Celeste.conf` which lives in `~/.config/Sky-Meyg` on Linux, or `Celeste.ini` which lives in `%APPDATA%\Sky-Meyg` on Windows. You will need to build an initial one before you can use Celeste. And example looks like the following.

```
[Authorization]
Administrator=
Token=
JoinDelay=3000

[Window]
BackgroundColor=#d8000000
AccentColor=#ff49218b
HelpCooldown=600000

[Vibe]
Playlist=/home/herb/Documents/Music/vibe.m3u

[Events]
Arrival=/home/herb/Music/Social Media/Twitch/icq.mp3
Thinking=/home/herb/Music/Social Media/Twitch/kwirk.mp3
```

Set `Administrator` to your Twitch username and `Token` to your app's OAuth token, which can be generated by following the instructions in the [Twitch Developer Documentation](https://dev.twitch.tv/docs) for [Authentication](https://dev.twitch.tv/docs/authentication).

### Commands

A file named `commands.json` will be generated in `~/.local/share/Sky-Meyg/Celeste` on Linux, or `%APPDATA%\Sky-Meyg\Celeste` on Windows. It will initially be an empty array. This is where you define the commands the Celeste can recognize and what the reponse will be. Here is an example.

```
[{
    "command": "gandalf",
    "description": "Celebrate with Epic Sax Gandalf",
    "type": "video",
    "random": false,
    "path": "/home/herb/Videos/Social Media/Twitch/gandalf.mp4"
},
{
    "command": "tiktok",
    "description": "Play a random TikTok video from Herb's favorites",
    "type": "video",
    "random": true,
    "path": "/home/herb/Videos/Social Media/TikTok"
}]
```
