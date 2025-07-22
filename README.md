
# SpatGRIS

SpatGRIS is a sound spatialization software that frees composers and sound designers from the constraints of real-world speaker setups.

With the ControlGRIS plugin distributed with SpatGRIS, rich spatial trajectories can be composed directly in your DAW and reproduced in real-time on any speaker layout. It is fast, stable, cross-platform, easy to learn and works with the tools you already know.

SpatGRIS supports any speaker setup, including 2D layouts like quad, 5.1 or octophonic rings, and 3D layouts like speaker domes, concert halls, theatres, etc. Projects can also be mixed down to stereo using a binaural head-related transfer function or simple stereo panning.

It can handle up to 256 inputs and outputs simultaneously and features 3D and 2D views that help visualizing sources motions and monitor sound activity.

SpatGRIS is developed by the _Groupe de recherche en immersion spatiale_ (GRIS) at Université de Montréal and is in active development. Updates are published on a regular basis.

- [Using a virtual audio device](#using-a-virtual-audio-device)
- [Building](#building)
- [Running](#running)
- [Using custom OSC interfaces](#using-custom-OSC-interfaces)

#### Note on the Windows installation program

Installing version 3.2.10 (or earlier) on a newer version may fail due to the file format of the ControlGris VST3 plugin being impossible to update. If an error occurs, manually deleting ControlGris.vst3 (C:\Program Files\Common Files\VST3\ControlGris.vst3) will allow the installer to proceed.

## Using a virtual audio device

If you want to use SpatGRIS and ControlGRIS on the same computer, you will need a virtual audio device that can route the audio from your DAW to SpatGRIS.

#### MacOS

We officially support the use of [BlackHole](https://github.com/ExistentialAudio/BlackHole). The 256, 128 and 64 channel versions are distributed with SpatGRIS.

#### Windows

Because ASIO does not allow interfacing with two different audio devices simultaneously, the best working configuration is to use [JACK](https://jackaudio.org/) with [ASIO4ALL](https://asio4all.org/).

If you are a [Reaper](https://www.reaper.fm/) user, ReaRoute may be a viable alternative. Note that sound has to be sent back to Reaper.

#### Linux

While we do not support a specific routing solution on Linux, there are a lot of different ways of creating loopback audio ports with either JACK, PipeWire, ALSA or PulseAudio.

## Building

Starting with version 3.3.0, the 3D speaker setup view of SpatGRIS is a seperate application: [SpeakerView](https://github.com/GRIS-UdeM/SpeakerView). It is included in the SpatGRIS installer.

To function correctly, the SpeakerView executable (and the SpeakerView.pck file under Windows and Linux) must be placed in the same folder as SpatGris. SpeakerView is independent of SpatGris, but designed to be controlled by SpatGris using the UDP protocol.

On Linux, the name of the executable must be SpeakerView.x86_64.

#### 1. Install dependencies

Download and extract [Juce 8.0.8](https://github.com/juce-framework/JUCE/releases/tag/8.0.8)

##### Additional dependencies on Linux :

```bash
sudo apt-get install clang++-14 ladspa-sdk libasound2-dev \
libcurl4-openssl-dev libfreetype6-dev libx11-dev libxcomposite-dev \
libxcursor-dev libxinerama-dev libxrandr-dev mesa-common-dev libjack-dev
```

#### 2. Generating project files

```bash
cd <SpatGRIS-path>
<path-to-projucer> --resave SpatGRIS.jucer
```

#### Compiling

Go to the generated `Builds/` folder.

On Windows, use the Visual Studio 2022 solution file.

On MacOS, use the Xcode project. You will have to supply your own developer ID to XCode.

On Linux :

```bash
cd Builds/LinuxMakeFile
make CONFIG=Release CXX=clang++-14 -j 8
```

## Running

SpatGRIS must be launched from the project root directory.

## Using custom OSC interfaces

OSC can be sent directly to SpatGRIS without having to use ControlGRIS.

There is an example patch made for MAX 7 provided in this repo ([osc_max_example.maxpat](osc_max_example.maxpat)).

__Please note that angles are always measured clockwise, starting from the upstage center (the positive Y direction).__

The server address is always `/spat/serv`.

##### `pol` moves a source using polar coordinates in radians.

| #parameter | type   | allowed values | meaning         |
| :---       | :---   | :---           | :---            |
| 1          | string | `pol`          | -               |
| 2          | int    | [1, 128]       | Source index    |
| 3          | float  | any            | azimuth angle   |
| 4          | float  | any            | elevation angle |
| 5          | float  | [-3.0, 3.0]    | radius          |
| 6          | float  | [0, 1]         | Horizontal span |
| 7          | float  | [0, 1]         | Vertical span   |

ex : The message `/spat/serv pol 7 0.0 0.78 0.5 0.1 0.2` moves the source #7 in the front at half elevation and placed at half the distance from the origin, with an horizontal span of 10% and a vertical span of 20%.

#### `deg` moves a source using polar coordinates in degrees.

| index | type   | allowed values | meaning         |
| :---  | :---   | :---           | :---            |
| 1     | string | `deg`          | -               |
| 2     | int    | [1, 128]       | Source index    |
| 3     | float  | any            | azimuth angle   |
| 4     | float  | any            | elevation angle |
| 5     | float  | [-3.0, 3.0]    | radius          |
| 6     | float  | [0, 1]         | Horizontal span |
| 7     | float  | [0, 1]         | Vertical span   |

ex : The message `/spat/serv deg 7 -90.0 45.0 0.5 0.1 0.2` moves the source #7 at the extreme left, at half elevation and half the distance of the space, with an horizontal span of 10% and a vertical span of 20%.

#### `car` moves a source using cartesian coordinates.

| index | type   | allowed values | meaning         |
| :---  | :---   | :---           | :---            |
| 1     | string | `car`          | -               |
| 2     | int    | [1, 128]       | Source index    |
| 3     | float  | [-1.66, 1.66]  | x (left/right)  |
| 4     | float  | [-1.66, 1.66]  | y (back/front)  |
| 5     | float  | [-1.66, 1.66]  | z (down/up)     |
| 6     | float  | [0, 1]         | Horizontal span |
| 7     | float  | [0, 1]         | Vertical span   |

ex : The message `/spat/serv car 7 1.0 1.0 1.0 0.0 0.0` moves the source #7 at the top right corner, with no horizontal or vertical spans.

#### `clr` clears a source's position.

| index | type   | allowed values | meaning      |
| :---  | :---   | :---           | :---         |
| 1     | string |`clr`           | clear        |
| 2     | int    | [1, 128]       | Source index |

ex : The message `/spat/serv clr 7` clears the seventh source's position.

#### `alg` sets a source's hybrid spatialization mode.

| index | type   | allowed values   | meaning      |
| :---  | :---   | :---             | :---         |
| 1     | string | `alg`            | -            |
| 2     | int    | [1, 128]         | Source index |
| 3     | string | `dome` or `cube` | Algorithm    |

ex : The message `/spat/serv alg 7 cube` sets the seventh source's spatialization algorithm to "cube" (only works in _hybrid_ mode).
