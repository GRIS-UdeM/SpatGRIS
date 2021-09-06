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

Download and extract [Juce 6.0.8](https://github.com/juce-framework/JUCE/releases/tag/6.0.8)

##### Additional dependencies on Linux :

```bash
sudo apt-get install clang++-10 ladspa-sdk freeglut3-dev libasound2-dev \
libcurl4-openssl-dev libfreetype6-dev libx11-dev libxcomposite-dev \
libxcursor-dev libxinerama-dev libxrandr-dev mesa-common-dev libjack-dev \
libfftw3-dev
```

##### Additional dependencies on Windows :

Usage of [vcpkg](https://github.com/microsoft/vcpkg) is highly recommended on Windows.

```bash
# as an admin
vcpkg install freeglut:x64-windows
vcpkg install fftw3:x64-windows
vcpkg integrate install
```

##### Additional dependencies on MacOS :

Usage of [Homebrew](https://brew.sh/) is highly recommended on MacOS.

```bash
brew install fftw
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

## Using alternative OSC interfaces

OSC can be sent directly to SpatGRIS without going through ControlGRIS.

The server address is `/spat/serv`.

#### Moving a source

SpatGRIS expects an `iffffff` list (1 integer and 6 floats).

1. (i) Source index (starting at 0).
2. (f) Azimuth angle (between 0 and 2π).
3. (f) Elevation :
	- In DOME mode : elevation angle (between 0 and π/2, where 0 is the pole).
	- In CUBE mode : source height (between 0 and π/2, where 0 is the ground level and π/2 is the maximum height).
4. (f) Azimuth span (between 0 and 2).
5. (f) Elevation span (between 0 and 0.5).
6. (f) Radius :
	- In DOME mode : __unused__ : must always be set to 0.
	- In CUBE mode : the distance between the origin and the source __projected onto the ground plane__. For example, this means that a source located at [1,0,0.5] would have a radius of 1. 
7. (f) __reserved__ : must always be set to 0.

#### Resetting a source

SpatGRIS expects an `si` list (1 string and 1 integer).

1. (s) The string "reset"
2. (i) The source index (starting at 0).
