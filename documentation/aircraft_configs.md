Aircraft config files are a JSON file that describes how an aircraft will interface with the IFR-1 controller.

The JSON file is structured as follows:

### Installation
Configuration files should be placed in a directory named `configs` located in the plugin's root folder.

Standard directory structure:
```
Resources/plugins/IFR1_Flex/
├── 64/
│   └── lin.xpl (the plugin binary)
└── configs/
    ├── generic.json
    ├── cessna_172.json
    └── ...
```

If the plugin is installed as a single file (flat layout), the `configs` directory should be next to it:
```
Resources/plugins/
├── ifr1flex.xpl
└── configs/
    ├── generic.json
    └── ...
```

### Fallback
A config file can be designated as the fallback configuration by adding a `"fallback": true` node at the top level. This configuration will be used if no other config file matches the loaded aircraft. Only one fallback file should be present.

```json
{
  "fallback": true,
  "modes": { }
}
```

### Aircraft
Each config file (except a fallback one) must have an `aircraft` node which is a list of strings. These strings are partial matches for the aircraft filename (the `.acf` file) in X-Plane.

```json
{
  "aircraft": [
    "Cirrus SR22",
    "CirrusSF50"
  ],
  "modes": {
    "com1": {
      "swap": {
        "short-press": {
          "type": "command",
          "value": "sim/radios/com1_standy_flip"
        }
      }
    }
  }
}
```

### Modes
Each aircraft will have a list of modes.  Some may be missing, which means they are not implemented.

com1  
com2  
nav1  
nav2  
fms1  
fms2  
ap  
xpdr  
hdg (this is a shifted com1)
baro (this is a shifted com2)
crs1 (this is a shifted nav1)
crs2 (this is a shifted nav2)
fms1-alt (this is a shifted fms1)
fms2-alt (this is a shifted fms2)
ap-alt (this is a shifted ap)
xpdr-mode (this is a shifted xpdr)

Each mode has a list of one or more actions.  There must be at least one action for each implemented mode.
direct-to
menu
clr
ent
ap
hdg
nav
apr
alt
vs
swap
inner-knob
outer-knob
inner-knob-button

Each action has an action type.  Each action must have a type.  There can be multiple types of actions for a single action.
short-press
long-press
rotate-clockwise
rotate-counterclockwise

Each action type has a response type.  There must be one of these for each action type.
command (which will send a command to the simulator)
dataref-set (which will set a dataref to a specific value)
dataref-adjust (which will adjust a dataref by a specific value)

Each response type will have a response value.  The value will depend on the response type.
For a command, the response value will be the command to send to the simulator.
For a dataref-set or dataref-adjust, the response value will be the dataref to set or change.

Each response type may have an adjustment value.  This is used for response types of dataref-set and dataref-change.
When the response type is a dataref-set, the value is a single value to set the dataref to.
When the response type is a dataref-adjust, the value is the amount to change the dataref by.

Each response type may have a minimum value.  This is used for response types of dataref-adjust. If there is a minimum value, there must also be a maximum value.

Each response type may have a maximum value.  This is used for response types of dataref-adjust. If there is a maximum value, there must also be a minimum value.

If there is a minimum and maximum value, there will be a limit type.
Limit types are either stop or wrap.
If the limit type is stop, the value will be clamped to the minimum and maximum values.
If the limit type is wrap, the value will be wrapped around the minimum and maximum values.

### Output
Each aircraft may have an output node.  This node defines how each controller LED will be controlled.
The output may have a list of LEDs.  Not every LED will be implemented, and there may be no LEDs implemented at all, in which case the output node will be omitted.

LEDs:
ap
hdg
nav
apr
alt
vs

All LEDs will default to off unless they are explicitly turned on by a test.

Each LED will have one or more tests.
A test is a dataref that will be checked for a specific value or bit.
An LED may have more than one test.
The first test that is met will take precedence over later tests.

Each test will have a minimum and maximum value, or a bit. 
If a bit is provided, the dataref value will be checked if that bit is set (using bitwise AND).
If minimum and maximum values are provided, and the dataref value is outside of these values, the LED will be turned off.
If both are provided, the bit check takes precedence (this depends on implementation, but usually you'd use one or the other).

Each test will have a LED mode.
The LED mode will determine how the LED will be lit when the test is met.
solid = the LED is on solidly when the test is met
blink = the LED blinks when the test is met

Each LED mode may have a blink rate.
The blink rate will determine how often the LED will blink per second.
The default blink rate is .5 Hz.
