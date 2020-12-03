# SpatGRIS

SpatGRIS lets composers and sound designers assign motions to sounds that are decoupled from any specific speaker layout.

It is developed by the _Groupe de recherche en immersion spatiale_ (GRIS) at Université de Montréal. When used in tandem with [ControlGRIS](https://github.com/GRIS-UdeM/ControlGris) (an open-source plugin for digital audio workstations), virtual trajectories can be assigned to sounds directly in the DAW. The audio is spatialized in realtime by SpatGRIS, according to the current speaker setup.

SpatGRIS works in 2D for dome-like speaker layouts or in 3D for arbitrary speaker layouts. It can handle up to 256 inputs and outputs (if you think your computer can handle it), with as many VU-meters as needed. It has 3D and 2D view windows and offers the possibility of viewing sound activity in the 3D window.

This software is in active development. Updates are published on a regular basis.

## Virtual audio device

If you want to use SpatGRIS and ControlGRIS on the same computer, you will need a virtual audio device that can route the audio from your DAW to SpatGRIS.

#### MacOS

We officially support using [BlackHole](https://github.com/ExistentialAudio/BlackHole). A 128 channels version [is made available with every SpatGRIS release](https://github.com/GRIS-UdeM/ServerGRIS/releases).

#### Windows

Note : ASIO dissalows interfacing with two different devices simoultaneously. If you are internally routing audio on Windows, you will need to find a non-ASIO virtual interface. ASIO should be reserved for setups where SpatGRIS uses the same audio interface for input and ouput and operates on a different machine than ControlGRIS.

There is a donationware called [VB-CABLE Virtual Audio Device](https://vb-audio.com/Cable/) that works with SpatGRIS, although it seems to be limited to 32 channels.

If you know of any other solution (preferably open sourced) that works better on Windows, __please tell us all about it__!

#### Linux

Their are numerous ways of creating loopback audio ports in Linux with ALSA, PulseAudio or JACK. While we do not provide a standard way of doing it, power users should have no problem figuring it out ;-).

## Dependencies

Linux :

```bash
sudo apt-get install clang++-10 ladspa-sdk freeglut3-dev libasound2-dev \
libcurl4-openssl-dev libfreetype6-dev libx11-dev libxcomposite-dev \
libxcursor-dev libxinerama-dev libxrandr-dev mesa-common-dev
```

There is no external dependencies for Windows and MacOS.

## Build

1. [Download Juce 6](https://juce.com/get-juce).
2. Generate the project files.

```bash
cd <ServerGRIS-path>
<path-to-projucer> --resave SpatGRIS.jucer
```

3. Go to the generated `Builds/` folder. On Windows, use the Visual Studio 2019 solution file. On MacOS, use the Xcode project. On Linux :

```bash
cd Builds/LinuxMakeFile
make CONFIG=Release CXX=clang++-10
```

## Run

The software must be launched from the project root directory.
