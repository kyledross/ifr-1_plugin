# Aircraft Configuration Guide

The Octavi IFR-1 Flex plugin uses JSON configuration files to map hardware events to X-Plane commands and datarefs. This guide explains the structure and available options for creating your own aircraft configurations.

## File Location and Loading

Configuration files must be placed in a directory named `configs` located in the plugin's root folder.

### Directory Structure
```
Resources/plugins/IFR1_Flex/
├── 64/
│   └── lin.xpl (the plugin binary)
└── configs/
    ├── generic.json
    ├── cessna_172.json
    └── ...
```

### Aircraft Matching
Each configuration file (except the fallback) must contain an `aircraft` array. The plugin matches these strings against the aircraft's `.acf` filename. If any string in the array matches (partial match), the configuration is loaded.

```json
{
  "aircraft": ["Cessna_172SP_G1000", "N844X"],
  "modes": { ... }
}
```

### Fallback Configuration
A configuration can be designated as the fallback by adding `"fallback": true` at the top level. This configuration will be used if no other file matches the loaded aircraft.

### Debugging
To enable verbose logging for a specific aircraft, add `"debug": true` at the top level. Details about event processing and condition evaluation will be logged to X-Plane's `Log.txt`.

---

## Modes and the Shifted State

The IFR-1 controller has 8 primary modes, selected by the physical mode knob. Each mode also has a "shifted" state, toggled by a **long-press (0.5 seconds) on the inner knob**.

The plugin uses different identifiers for these states in the JSON:

| Selector Position | Primary Mode Identifier | Shifted Mode Identifier |
| :--- | :--- | :--- |
| **COM 1** | `com1` | `hdg` |
| **COM 2** | `com2` | `baro` |
| **NAV 1** | `nav1` | `crs1` |
| **NAV 2** | `nav2` | `crs2` |
| **FMS 1** | `fms1` | `fms1-alt` |
| **FMS 2** | `fms2` | `fms2-alt` |
| **AP** | `ap` | `ap-alt` |
| **XPDR** | `xpdr` | `xpdr-mode` |

---

## Defining Actions

Within each mode, you map **Controls** to **Actions** based on **Event Types**.

### Hardware Controls
- **Knobs**: `outer-knob`, `inner-knob`
- **Knob Button**: `inner-knob-button`
- **Right Buttons**: `direct-to`, `menu`, `clr`, `ent`
- **Center Buttons**: `swap`
- **Bottom Buttons**: `ap`, `hdg`, `nav`, `apr`, `alt`, `vs`

### Event Types
- **For Knobs**: `rotate-clockwise`, `rotate-counterclockwise`
- **For Buttons**: `short-press`, `long-press`

### Action Types
There are three types of actions you can trigger:

#### 1. `command`
Executes a standard X-Plane command.
- `value`: The command path (e.g., `sim/radios/com1_standy_flip`).

#### 2. `dataref-set`
Sets a dataref to a specific value.
- `value`: The dataref name.
- `adjustment`: The value to set (e.g., `1` for ON, `0` for OFF).

#### 3. `dataref-adjust`
Increments or decrements a dataref.
- `value`: The dataref name.
- `adjustment`: The amount to add (use negative for subtraction).
- `min`: (Optional) Minimum allowed value.
- `max`: (Optional) Maximum allowed value.
- `limit-type`: (Optional) `"clamp"` (default) or `"wrap"`.

### Array Datarefs
Many X-Plane datarefs are arrays (e.g., `sim/cockpit2/switches/panel_brightness_ratio` which has 4 values). You can access a specific index by appending `[index]` to the dataref name.

- Example: `sim/cockpit2/switches/panel_brightness_ratio[0]`
- This syntax works in `dataref-set`, `dataref-adjust`, and within `conditions`.

---

## Conditional and Multiple Actions

If you want an event to do different things depending on the aircraft's state, or to perform multiple actions, you can use an array of actions with `conditions`. 

### Execution Logic
- The plugin iterates through the array of actions.
- For each action, it evaluates the `condition` (or `conditions`).
- If the conditions are met, the action is executed.
- By default, after executing an action, the plugin **stops** and does not evaluate any further actions in the array for that event.
- If you want to continue to the next action even after a match, add `"evaluate-next-condition": true` to the condition object or the action itself.

### Condition Syntax
A condition checks a dataref value:
- `dataref`: The name of the dataref to check.
- `min` and `max`: (Required for range) Inclusive range the value must fall into.
- `bit`: (Alternative to range) A 0-indexed bit that must be set in the integer value.
- `evaluate-next-condition`: (Optional) If `true`, the plugin will continue to evaluate subsequent actions in the array even if this action's conditions were met and it was executed.

```json
"alt": {
  "short-press": [
    {
      "condition": { 
        "dataref": "sim/cockpit2/autopilot/altitude_mode", 
        "min": 5, "max": 5, 
        "evaluate-next-condition": true 
      },
      "type": "command",
      "value": "sim/autopilot/airspeed_up"
    },
    {
      "type": "command",
      "value": "sim/autopilot/altitude_hold"
    }
  ]
}
```

---

## LED Outputs

The `output` node controls the LEDs for the bottom buttons (`ap`, `hdg`, `nav`, `apr`, `alt`, `vs`).

### LED Configuration
Each LED can have multiple conditions. The first matching condition wins.
- `mode`: `"solid"` or `"blink"`.
- `blink-rate`: (Optional) Frequency in Hz. Defaults to `1.0`.

```json
"output": {
  "alt": {
    "conditions": [
      { "dataref": "sim/cockpit2/autopilot/altitude_mode", "min": 6, "max": 6, "mode": "solid" },
      { "dataref": "sim/cockpit2/autopilot/altitude_mode", "min": 5, "max": 5, "mode": "blink", "blink-rate": 2.0 }
    ]
  }
}
```

---

## Example Configuration

Here is a concise example showing a basic Com 1 setup with Heading sync.

```json
{
  "name": "Example Config",
  "aircraft": ["Cessna_172"],
  "modes": {
    "com1": {
      "swap": {
        "short-press": { "type": "command", "value": "sim/radios/com1_standy_flip" }
      },
      "outer-knob": {
        "rotate-clockwise": { "type": "command", "value": "sim/radios/stby_com1_coarse_up" },
        "rotate-counterclockwise": { "type": "command", "value": "sim/radios/stby_com1_coarse_down" }
      },
      "inner-knob": {
        "rotate-clockwise": { "type": "command", "value": "sim/radios/stby_com1_fine_up" },
        "rotate-counterclockwise": { "type": "command", "value": "sim/radios/stby_com1_fine_down" }
      }
    },
    "hdg": {
      "inner-knob-button": {
        "short-press": { "type": "command", "value": "sim/autopilot/heading_sync" }
      },
      "outer-knob": {
        "rotate-clockwise": {
          "type": "dataref-adjust",
          "value": "sim/cockpit/autopilot/heading_mag",
          "adjustment": 10, "min": 0, "max": 359, "limit-type": "wrap"
        }
      }
    }
  },
  "output": {
    "ap": {
      "conditions": [
        { "dataref": "sim/cockpit/autopilot/autopilot_mode", "min": 2, "max": 2, "mode": "solid" }
      ]
    }
  }
}
```
