# SpatGRIS

SpatGRIS is a sound spatialization software that frees composers and sound designers from the constraints of real-world speaker setups.

With the ControlGRIS plugin distributed with SpatGRIS, rich spatial trajectories can be composed directly in your DAW and reproduced in real-time on any speaker layout. It is fast, stable, cross-platform, easy to learn and works with the tools you already know.

SpatGRIS supports any speaker setup, including 2D layouts like quad, 5.1 or octophonic rings, and 3D layouts like speaker domes, concert halls, theatres, etc. Projects can also be mixed down to stereo using a binaural head-related transfer function or simple stereo panning.

It can handle up to 128 inputs and outputs simultaneously and features 3D and 2D views that help visualizing sources motions and monitor sound activity.

SpatGRIS is developed by the _Groupe de recherche en immersion spatiale_ (GRIS) at Université de Montréal and is in active development. Updates are published on a regular basis.

- [Using a virtual audio device](#using-a-virtual-audio-device)
- [Building](#building)
- [Running](#running)
- [Using alternative OSC interfaces](#using-alternative-OSC-interfaces)

![alt text](https://samuelbeland.com/gris/spatGRIS_3.1.png "SpatGRIS")

## Using a virtual audio device

If you want to use SpatGRIS and ControlGRIS on the same computer, you will need a virtual audio device that can route the audio from your DAW to SpatGRIS.

#### MacOS

We officially support using [BlackHole](https://github.com/ExistentialAudio/BlackHole). A 128 channels version is distributed alongside SpatGRIS.

#### Windows

If you are a [Reaper](https://www.reaper.fm/) user, ReaRoute seems to be the best-working virtual interface available at this time on Windows. Because ASIO dissalows interfacing with two different audio devices simoultaneously, sound has to be sent back to Reaper.

If you use an other DAW, there is a donationware called [VB-CABLE Virtual Audio Device](https://vb-audio.com/Cable/) that _sorta_ works, although it seems to have trouble staying in sync with SpatGRIS and is limited to 32 channels.

#### Linux

While we do not support a specific routing solution on Linux, there are a lot of different ways of creating loopback audio ports with either JACK, ALSA or PulseAudio.

## Building

#### 1. Installing dependencies

Download and extract [Juce 6.1.2](https://github.com/juce-framework/JUCE/releases/tag/6.1.2)

##### Additional dependencies on Linux :

```bash
sudo apt-get install clang++-10 ladspa-sdk freeglut3-dev libasound2-dev \
libcurl4-openssl-dev libfreetype6-dev libx11-dev libxcomposite-dev \
libxcursor-dev libxinerama-dev libxrandr-dev mesa-common-dev libjack-dev
```

##### Additional dependencies on Windows :

Usage of [vcpkg](https://github.com/microsoft/vcpkg) is highly recommended on Windows.

```bash
# as an admin
vcpkg install freeglut:x64-windows
vcpkg integrate install
```

#### 2. Generating project files

```bash
cd <ServerGRIS-path>
<path-to-projucer> --resave SpatGRIS.jucer
```

#### Compiling

Go to the generated `Builds/` folder.

On Windows, use the Visual Studio 2019 solution file.

On MacOS, use the Xcode project. You will have to supply your own developer ID to XCode.

On Linux :

```bash
cd Builds/LinuxMakeFile
make CONFIG=Release CXX=clang++-10 -j 8
```

## Running

SpatGRIS must be launched from the project root directory.

## Using custom OSC interfaces

OSC can be sent directly to SpatGRIS without having to use ControlGRIS.

__Please note that angles are always measured clockwise, starting from the upstage center (the positive Y direction).__

The server address is always `/spat/serv`.

##### `pol` moves a source using polar coordinates in radians.

| #parameter | type   | allowed values | meaning         |
| :---       | :---   | :---           | :---            |
| 1          | string | `pol`          | coordinate type |
| 2          | int    | [1, 128]       | Source index    |
| 3          | float  | any            | azimuth angle   |
| 4          | float  | [0, π/2]       | elevation angle |
| 5          | float  | [0, 2.56]      | radius          |
| 6          | float  | [0, 1]         | Horizontal span |
| 7          | float  | [0, 1]         | Vertical span   |

ex : The message `/spat/serv pol 7 0.0 0.78 0.5 0.1 0.2` moves the source #7 in the front at half elevation and placed at half the distance from the origin, with an horizontal span of 10% and a vertical span of 20%.

#### `deg` moves a source using polar coordinates in degrees.

| index | type   | allowed values | meaning         |
| :---  | :---   | :---           | :---            |
| 1     | string | `deg`          | coordinate type |
| 2     | int    | [1, 128]       | Source index    |
| 3     | float  | any            | azimuth angle   |
| 4     | float  | [0, 90]        | elevation angle |
| 5     | float  | [0, 2.56]      | radius          |
| 6     | float  | [0, 1]         | Horizontal span |
| 7     | float  | [0, 1]         | Vertical span   |

ex : The message `/spat/serv deg 7 -90.0 45.0 0.5 0.1 0.2` moves the source #7 at the extreme left, at half elevation and half the distance of the space, with an horizontal span of 10% and a vertical span of 20%.

#### `car` moves a source using cartesian coordinates.

| index | type   | allowed values | meaning         |
| :---  | :---   | :---           | :---            |
| 1     | string | `car`          | coordinate type |
| 2     | int    | [1, 128]       | Source index    |
| 3     | float  | [-1.66, 1.66]  | x (left/right)  |
| 4     | float  | [-1.66, 1.66]  | y (back/front)  |
| 5     | float  | [0, 1]         | z (down/up)     |
| 6     | float  | [0, 1]         | Horizontal span |
| 7     | float  | [0, 1]         | Vertical span   |

ex : The message `/spat/serv car 7 1.0 1.0 1.0 0.0 0.0` moves the source #7 at the top right corner, with no horizontal or vertical spans.

#### `clr` clears a source's position.

| index | type   | allowed values | meaning      |
| :---  | :---   | :---           | :---         |
| 1     | string |`clr`          | clear        |
| 2     | int    | [1, 128]       | Source index |

ex : The message `spat/serv clr 7` clears the seventh source's position.