# SpatGRIS2
SpatGRIS2 is developed by the Groupe de recherche en immersion spatiale (GRIS) at Université de Montréal. The software is dedicated to sound spatialization in 2D (plane mode, X and Y axis) and 3D (with vertical mode, Z axis). It includes a Speaker Setup design page and the movements are controled by an external OSC source, namely ControlGRIS, a plugin for Audio Unit and VST DAWs. 
The Server works on Jack and includes 256 inputs and outputs with as many VU-meters as needed.
The Server has a 3D and 2D view window and offers the possibility of viewing the sound activity in the 3D window.
The software is in its development phase and updates are published on a regular basis.

## Building the SpatGRIS2 on Debian (Ubuntu)

### Install dependencies

```
sudo apt-get install clang git ladspa-sdk freeglut3-dev g++ libasound2-dev libcurl4-openssl-dev libfreetype6-dev libjack-jackd2-dev libx11-dev libxcomposite-dev libxcursor-dev libxinerama-dev libxrandr-dev mesa-common-dev webkit2gtk-4.0 juce-tools jackd2
```

### Configure Jack realtime Scheduling

Make sure that you are part of the group "audio" or "jackuser".

```bash
sudo usermod -a -G audio <username>
```

If this does not work or if the group does not exists, follow the steps described [in the JACK FAQ](https://jackaudio.org/faq/linux_rt_config.html).

### Download Juce

To build the SpatGRIS2, you'll need Juce 5, download it from https://shop.juce.com/get-juce/download page.

### Clone SpatGRIS2 sources

```
git clone https://github.com/GRIS-UdeM/ServerGRIS.git
```

### Build the app

1. Start the Projucer app, open the SpatGRIS2.jucer file and save the project. This step must be done each time the structure of the project changes (new files, new JUCE version, etc.).

```
cd /path/to/JUCE/folder
./Projucer
```

After saving the project, you can quit the Projucer.

2. Go to the SpatGRIS2 Builds folder and compile the app.

```
cd ServerGRIS/Builds/LinuxMakeFile
make CONFIG=Release
```

### Run the SpatGRIS2 (from LinuxMakeFile directory)

```
./build/SpatGRIS2
```
