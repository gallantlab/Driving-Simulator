Driving Simulator for fMRI
===============
The driving simulator software code for Zhang et al. 2026, built in the Unreal Engine and makes extensive use of code from the [Carla Simulator](https://github.com/carla-simulator/carla)

Precompiled Binaries
--------------------

Getting precompiled binaries is the easiest way to look at the end product. 
To do so, use the launcher provided in the releases page (or build the launcher from scratch), which can then download the executables. 
The launcher can be used to configure various game settings at launch.

See [Experiment.md](Experiment.md) on how the experiment is typically run.

Building from Source
--------------------

We have built the simulator on Ubuntu 18.04 and 24.04, and Windows 10 and 11. 
Most of our Linux development happened on 18.04; for building on 24.04, see [24.04-specific notes](Ubuntu-24-notes.md).

Much of the code was developed directly on top of Carla 0.8.4, so there are a lot of leftover direct references to this project as "Carla."

- Install Unreal Engine 4.18 (you will need an Epic Games account)
  - Linux: join the Epic Games org, pull the UE repo on the 4.18 branch, and build following instructions [here](https://docs.unrealengine.com/en-US/SharingAndReleasing/Linux/BeginnerLinuxDeveloper/SettingUpAnUnrealWorkflow/index.html)
  - Windows: download the Epic Games launcher and install Unreal Engine 4.18
- Install and build the Unreal Project
  - Pull this repo
  - grab assets with `Utils/download_from_gdrive.py`
  - Run `Setup.sh`/`Setup.bat`
  - Build the Carla plugin
    - Linux: run Rebuild.sh
    - Windows: right click on the Unreal project, use "generate project files" to generate a VS solution, and use VS to build the solution
- To open the project, use the unreal project file
- To build eith updated files:
  - Linux: run `Rebuild.sh`
  - Windows: rebuild in VS
- To run instance without the editor open
  - Linux: run `Rebuild.sh`, run `Package.sh`, and the run the sh file in the Dist folder
  - Windows: change the debug mode to `DebugGame` instead of `DebugGame Editor` (you will need to have cooked files before this)
- Packaging
  - Linux: edit `Package.sh` to set the build config, and then run `Package.sh`
  - Windows: from UE4 Editor, File->Package for windows

Useful links and tips for common problems when building from source
- [UE4_ROOT is not defined](https://github.com/carla-simulator/carla/issues/658)
- ['carla/carla_server.h' file not found](https://github.com/carla-simulator/carla/issues/146)
- [clang++:not found](https://github.com/carla-simulator/carla/issues/2635)
- [Finishing your build](https://carla.readthedocs.io/en/stable/carla_server/)
- [Boost 403 Forbidden](https://github.com/carla-simulator/carla/issues/2664)
- If `..PythonClient/` directory is not found, manually mkdir

Note that because of licensing, we cannot include many of assets used to create the map in the compiled version of the driving simulator. Please get in touch if you would like to work out a way to get these assets.

Recommended dev software
--------------------

IDE: Visual Studio (Windows), CLion (Linux)

Git: [Rebased](https://github.com/DetachHead/rebased/)

License
-------

This project is licensed under the BSD 3-Clause License. See [LICENSE](LICENSE) for details.

This project is built on the [CARLA Simulator](https://github.com/carla-simulator/carla), developed by the Computer Vision Center (CVC) at the Universitat Autonoma de Barcelona (UAB) and released under the MIT License. The original CARLA source code and all portions derived from it remain under the MIT License as specified in the LICENSE file.
