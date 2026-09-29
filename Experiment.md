Running the experiment in the MRI
=================================
A brief overview of how the game is run as an experiment in the fMRI

Overview
--------

The experiment runs in the `CarlaUE4` program. Each experiment has a config file that is passed to the executable, along with the map and configuration for that experiment. To make this process easier, the Launcher provides an interface to specify the config file and graphics settings.

In the experiemnt, the subject drives with a steering wheel and pedals, and requires a user interface device that enumerates to the computer as a gamepad/joystick and provides analog readouts in the Left Trigger (brake), Right Trigger (throttle) and Left Stick X (steer) channels.

Each run produces a demofile and a screen capture video. It is assumed that eyetracking is running in the background, but the software does not interact with it.

A demofile is a recording of gameplay that contains everything required to reconstruct it, and features are extracted from it. Visual content is not taken from the demofile. OBS records the screen buffer directly to video instead.

Equipment
---------

* OBS Studio with its websocket server running on `localhost:4444` (the default)
* A 4-button buttonbox
* A MRI-compatiable steering wheel and pedal set

Controls
--------

### Buttonbox

* `1` toggles forward/reverse gears.
* `2` engages the hand brake. It serves as a backup in case the brake pedal is unreliable.
* `3` shows the navigation HUD for 2 seconds, for when the subject needs a hint.
* `4`, when held down, lets the subject indicate that they are lost and want to end the trial.

### Keyboard (controls for the experimenter to interrup the participant)

* `Q` toggles forward/reverse.
* `R` resets the environment after unforeseen physics or navmesh errors.
* `Enter` makes the car jump, which can free it when it is stuck.
* `Tab` switches between first-person and third-person view. The subject should be in first-person view during the experiment.

Procedure
---------

### Pre-experiment

1. Start the game with the launcher.
2. Start OBS with the button in the launcher.
3. In the OBS Sources list, check that it is configured to take the source as the game window. The preview should show live video from the driving experiment.

### For each run

1. Start eyetracking.
2. Open the game menu with `Esc` and press `Start Demo Recording`.
3. Run the fMRI sequence.
4. Press `Stop Demo Recording` in the game menu.
5. Stop the raw eyetracking video recording.

Starting and stopping the demo recording should start and stop the OBS screen capture automatically through the websocket connection. 
If the screen capture does not start or stop, you will need to manually start/stop it through OBS.

Troubleshooting
---------------

If the mouse cursor is visible, click on the game window so that the game captures the mouse.

If the subject gets stuck because of an AI or physics bug, stop the demo recording and press `R` to reset the world. Then start a new MRI run.

If a demo recording was not started, the first TTL from the scanner starts one. If a demo recording was not stopped, the experiment stops it after about 10 seconds without TTLs.

Configuration
-------------

Each experiment's configuration is stored in an ini file named after the experiment in the `Config\Experiment Config` folder.

`SecondsBeforeAISteering` sets how many seconds without steering input pass before the lane keep assist kicks in. Zero disables auto-steering.

Set `AutoEyetrackingCalibration` to `true` to have the game display eyetracking calibration points at the beginning of each run.
