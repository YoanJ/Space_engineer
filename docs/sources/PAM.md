Updates:
1.3.1: 2019-12-20
- Updated to newest SE version

Full changelog


Info:
The development of PAM is stopped due to i stopped playing SE.

The script runs well with the latest SE version, if something doesn't work it could be due to mods.
Please understand that i'm not active here to answer your questions.
If any future SE update will break PAM i will do my best to fix that.

Big thanks for all your support, cheers!


Guide:

For more details, additional informations and troubleshooting click here:
https://steamcommunity.com/sharedfiles/filedetails/?id=1553126390


Description:

This script was started as "mining only" script (thats why its called "Auto Miner")
But in the meantime i have added more and more functions, now it is a multifunctional script for different types of tasks.
Currently there are 4 different script modes:

Mining mode: fully automated mining
Grinding mode: fully automated grinding
Shuttle mode: fully automated transportation of items and players
Station mode: Stationary monitoring system for your ships

The script uses a specified path to fly between your docking station and the job position. (It does not use the buggy pathfinding function of the remote block)
It is designed for planets and space. It works with small and large ships.
The usage is pretty simple and with all script modes similar:

-Dock your ship to a connector
-Start recording the path
-Fly to the job area
-Stop recording
-Setup the job and start the process

That's all.

The script starts executing the configured job, and flies fully automated between the positions by using the path.

General features
- Automated traveling between base and job area along the path (must be recorded befor)
- Works in space, on planets, with small and large ships
- Useful monitoring settings
- Useful behavior settings
- LCD menu, controlled by 3 commands (UP, APPLY, DOWN)
- Many commands to control it from distance

Note:
- PAM can only connect to stationary connectors, moving connectors are not supported.
- You can convert a carrier/mothership into a station to make it compatible with PAM
- Path can be recorded: dock to job position or: job position to dock, it does not matter

Script modes overview

This script has 4 different modes for different tasks.
The activated mode is depending on your ship tools or name tags:

Mining mode: Ship has drills
Grinding mode: Ship has grinders
Shuttle mode: Ship has no drills and no grinders
Controller mode: PB has the nametag: [PAM-Controller]

The mode will be enabled with the first run of the PB automatically.
You can reset the mode by running the "RESET" command.
The shuttle mode can be enabled manually with any ship by running the "SHUTTLE" command.

Mining mode:

Features
- Fully automated mining process
- Adjustable width, height and depth of the excavation
- visualisation with sensor range field

(More details in the guide)

Grinding mode:

Features
- mostly the same as mining mode, but with grinders instead of drills

(More details in the guide)

Shuttle mode:

Features
- Traveling between two connectors
- Possibilities to automate shuttle tasks (automated freighters, taxis and more)
- Individual & adjustable undock conditions
- Trigger timers on docking events

(More details in the guide)

Controller mode: ([PAM]-Controller)

The PAM-Controller is a system which you can place in your base or mothership. It gives you some special remote control functions about your PAM controlled ships.
It use the antenna broadcast to send and receive messages to and from your ships.

Features
- Displays the state of your ships on one lcd
- Dispalys the current inventory of the ships
- Makes possible to remote control the miners PAM-menu
- Can send commands to ships
- Multipe LCD modes available

(More details in the guide)

Ship setup:

NOTE: This script is not tested with mods, you can try it but maybe it will not work.

General
- Required blocks: Remote control, Programmable block, Connector, Thruster, Gyroscopes, Drills(mining mode), grinders(grinding mode)
- Optional blocks: LCD, Sensor(Required with grinding mode)
- Your docking station needs sorters with “Drain all” settings enabled, to drain or fill the ship

Remoteblock
- Must be aligned in flight direction
- The remotes down side must be facing in the same direction as your ships down side
- Add the [PAM] - tag when there are more than one available
- When it gets destroyed, the script stops every task. Take care about it.



LCD
- Add the [PAM] - tag
- You can have multiple LCDs

LCD's of cockpits and other blocks
- Add this Tag to the block with the LCD's: [PAM:<LCDIndex>]
- <LCDIndex> is the position of the LCD in the LCD Panel List of this block
- e.g: Cockpit [PAM:1]
- Important:
- Currently you have to adjust the font size manually

Landing gears
- Add the [PAM] -tag depending on the wanted behavior:
- if no landing gear is tagged: PAM controls all landing gears of the ship
- if one or more landing gears are tagged: PAM controls only landing gears with the tag

Sorters, Connectors, Batteries, Hydrogen tanks
- same tagging possiblity as with landing gears

Sensor
(Only mining and grinding mode)
- Must be aligned in mining direction (see the screenshot below, note the sensor rotation)
- The position on the ship does not matter
- Add the [PAM] - tag
- Enable "Show sensor fields" in the K menu (info tab)




Drills / Grinders
(Only mining and grinding mode)
(Only when using drills aligned in different directions:)
- Add the [PAM] - tag to one or more drills which are aligned in the mining-forward direction
- Do not add the tag to drills which are aligned in the other directions
- Drills with tag are acting as reference for the "working" direction
- The sidewards aligned drills will not count in the shaft size calculation

Programmable block
- Load the script, check and run it (no timer needed)
- Run the "RESET" command if the script is in the wrong mode
- Add 3 commands to toolbar: UP, APPLY, DOWN and navigate through menus


Important:
- All blocks have to be placed on the same grid as the PB.
- Make sure your ship has enough energy to supply the thrusters with 100% in every direction

Commands:

> Important:

[UP] Menu navigation up
[DOWN] Menu navigation down
[APPLY] Apply menu point

> Extra functions

[ALIGN] Align the ship to planet
[FULL] Simulate ship is full
[RESET] Reset data and script mode

(more in the guide)

Troubleshooting

You can find a troubleshooting section in the PAM-guide.

I hope you enjoy this script.
Please give me your feedback and let me know about bugs.