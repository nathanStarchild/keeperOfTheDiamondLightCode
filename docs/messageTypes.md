# Message Types Documentation

Complete reference for all message types used in the Keeper of the Diamond Light system.

## Table of Contents

### System & Protocol Messages (9xx Range)
- [999 - PING](#999---ping)
- [998 - PONG](#998---pong)
- [997 - ROLE](#997---role)
- [996 - BRIDGE_ANNOUNCE](#996---bridge_announce)
- [995 - NODE_COUNT](#995---node_count)
- [994 - TOTAL_LED_COUNT](#994---total_led_count)
- [993 - SWEEP_SPOT](#993---sweep_spot)

### [Presets](#presets-0-parameters)
- 1 - Upset MainState
- 2 - Double Rainbow
- 3 - Tranquility Mode
- 4 - Next Palette
- 5 - Tripper Trap Mode
- 6 - Ants Mode
- 7 - Shooting Stars
- 8 - Toggle House Lights
- 9 - Launch
- 14 - Noise Test
- 15 - Noise Fader
- 20 - Earth Mode
- 21 - Fire Mode
- 22 - Air Mode
- 23 - Water Mode
- 24 - Metal Mode
- 28 - Ripple Geddon
- 29 - Tail Time
- 30 - Rainbow Noise
- 31 - Flash Grid
- 39 - Rainbow Spiral
- 40 - Blender
- 51 - MG Noise Party
- 52 - MG Blob
- 53 - MG Random
- 54 - Enlightenment Achieved
- 63 - Node Counter
- [50 - Zoom to Colour](#50---zoom-to-colour) (has parameters)

### Configuration Commands (10-13, 16-19, 25-26, 32-34, 48)
- [10 - Set Step Rate](#10---set-step-rate)
- [11 - Set Fade Rate](#11---set-fade-rate)
- [12 - Set Brightness](#12---set-brightness)
- [13 - Set Number of Ripples](#13---set-number-of-ripples)
- [16 - Noise Length](#16---noise-length)
- [17 - Noise Speed](#17---noise-speed)
- [18 - Noise Fade Length](#18---noise-fade-length)
- [19 - Noise Fade Speed](#19---noise-fade-speed)
- [25 - Air Length](#25---air-length)
- [26 - Air Speed](#26---air-speed)
- [32 - Fire Decay](#32---fire-decay)
- [33 - Fire Speed](#33---fire-speed)
- [34 - Set Palette](#34---set-palette)
- [48 - Sweep](#48---sweep)

### Advanced Pattern Commands (35-43)
- [35 - Set Custom Palette Array](#35---set-custom-palette-array)
- [36 - Set Pattern Parameter](#36---set-pattern-parameter)
- [37 - Enable Single Pattern](#37---enable-single-pattern)
- [38 - Toggle Pattern State](#38---toggle-pattern-state)
- [41 - Enlightenment Callback](#41---enlightenment-callback)
- [42 - Skater Direction](#42---skater-direction)
- [43 - Launch Controller](#43---launch-controller)

### System Control (44-46, 55)
- [44 - Lock State](#44---lock-state)
- [45 - Disable Bored Timer](#45---disable-bored-timer)
- [46 - Enable Bored Timer](#46---enable-bored-timer)
- [55 - Start Return Timer](#55---start-return-timer)

---

## Message Type Details

## System & Protocol Messages (9xx Range)

### 999 - PING

**Purpose:** Time synchronization request sent from client to server.

**Parameters:** 1
- `t0` (uint64_t): Client timestamp in microseconds when ping was sent

**Format:**
```json
{
  "msgType": 999,
  "t0": 1234567890
}
```

**Parsed By:**
- Server (`server/app.js`)

**Response:** Server responds with message type 998 (PONG)

**Network Flow:**
- Client → Server only

**Notes:** Part of the time synchronization protocol. Client sends its timestamp, server responds with PONG containing both client's t0 and server's current time for RTT calculation.

---

### 998 - PONG

**Purpose:** Time synchronization response from server to client.

**Parameters:** 2
- `t0` (uint64_t): Original client timestamp from PING
- `serverTime` (uint64_t): Server's current time in milliseconds since server start

**Format:**
```json
{
  "msgType": 998,
  "t0": 1234567890,
  "serverTime": 5000
}
```

**Parsed By:**
- ESP32/ESP8266 clients (`client.h`)
- Mesh bridge (`meshBridge.h`)

**Network Flow:**
- Server → Client/Bridge only

**Notes:** Client calculates round-trip time (RTT) and time offset to synchronize with server. Critical for synchronized effects like launch sequences.

---

### 997 - ROLE

**Purpose:** Client identifies its role in the network (bridge, pyramid, browser).

**Parameters:** 1
- `role` (string): One of "bridge", "pyramid", "browser"

**Format:**
```json
{
  "msgType": 997,
  "role": "pyramid"
}
```

**Parsed By:**
- Server (`server/app.js`)

**Response:** 
- If role is "pyramid", server assigns sweepSpot (msgType 993) and broadcasts total LED count (msgType 994)

**Network Flow:**
- Client/Bridge/Browser → Server only

**Notes:** Sent once at connection time. Server uses this to track different device types and manage LED device counts. Pyramids get automatic sweepSpot assignment.

---

### 996 - BRIDGE_ANNOUNCE

**Purpose:** Mesh bridge announces itself to mesh network and provides count of non-mesh devices.

**Parameters:** 1
- `nonMeshCount` (uint8_t): Number of LED devices connected directly to server (not mesh)

**Format:**
```json
{
  "msgType": 996,
  "nonMeshCount": 3
}
```

**Parsed By:**
- Mesh nodes (`meshNode.h`)

**Network Flow:**
- Bridge → Mesh network only (broadcast)

**Notes:** Mesh nodes use this to calculate their sweepSpot (position + nonMeshCount). Bridge stores its ID in EEPROM so nodes can recognize it. Sent periodically and when nonMeshCount changes.

---

### 995 - NODE_COUNT

**Purpose:** Bridge reports mesh node count to server.

**Parameters:** 1
- `nodeCount` (uint32_t): Number of nodes currently on mesh network

**Format:**
```json
{
  "msgType": 995,
  "nodeCount": 12
}
```

**Parsed By:**
- Server (`server/app.js`)

**Response:** Server broadcasts TOTAL_LED_COUNT (msgType 994) to all clients

**Network Flow:**
- Bridge → Server only

**Notes:** Sent when mesh connection list changes. Server combines this with pyramid count to get total LED device count.

---

### 994 - TOTAL_LED_COUNT

**Purpose:** Server broadcasts total number of LED devices in the entire system.

**Parameters:** 3
- `totalLEDCount` (uint16_t): Total LED devices (mesh + pyramids)
- `meshCount` (uint16_t): Number of mesh nodes
- `pyramidCount` (uint16_t): Number of pyramid installations

**Format:**
```json
{
  "msgType": 994,
  "totalLEDCount": 15,
  "meshCount": 12,
  "pyramidCount": 3
}
```

**Parsed By:**
- ESP32/ESP8266 clients (`client.h`) - updates `mainState.nodeCount.plength`
- Mesh bridge (`meshBridge.h`) - stores and announces nonMeshCount to mesh

**Network Flow:**
- Server → All clients (broadcast)

**Notes:** Triggers `nodeCounter()` flash on clients. Used by launch() pattern for dynamic node count. Sent when device counts change. Debounced to prevent rapid flashing.

---

### 993 - SWEEP_SPOT

**Purpose:** Server assigns unique sweepSpot number to pyramid devices.

**Parameters:** 1
- `sweepSpot` (uint16_t): Sequential position number (0, 1, 2, ...)

**Format:**
```json
{
  "msgType": 993,
  "sweepSpot": 2
}
```

**Parsed By:**
- ESP32/ESP8266 clients (`client.h`)

**Network Flow:**
- Server → Pyramid clients only (unicast)

**Notes:** Pyramids get consecutive numbers starting at 0. Mesh nodes calculate their own (position + nonMeshCount). Server reassigns all pyramid sweepSpots when one disconnects.

---

## Presets (0 Parameters)

**All presets share the following characteristics:**

**Parameters:** 0

**Format:**
```json
{
  "msgType": [number]
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)
- Browser/Magic Button → Server → All devices

---

**1 - Upset MainState** calls `upset_mainState()` - no sync

**2 - Double Rainbow** calls `doubleRainbow()` - no sync

**3 - Tranquility Mode** calls `tranquilityMode()` - no sync

**4 - Next Palette** calls `next_mg_palette()` - no sync - sends setPalette (34) to sync mesh when triggered by magic button

**5 - Tripper Trap Mode** calls `tripperTrapMode()` - no sync

**6 - Ants Mode** calls `antsMode()` - no sync

**7 - Shooting Stars** calls `shootingStars()` - no sync

**8 - Toggle House Lights** toggles `mainState.houseLights.enabled` - no sync

**9 - Launch** sets stepRate=1, enables `mainState.launch.enabled`, calls `launch()` - **SYNCHRONIZED**

**14 - Noise Test** calls `noiseTest()` - no sync

**15 - Noise Fader** calls `noiseFader()` - no sync

**20 - Earth Mode** calls `earthMode()` - no sync

**21 - Fire Mode** calls `fireMode()` - no sync

**22 - Air Mode** calls `airMode()` - no sync

**23 - Water Mode** calls `waterMode()` - no sync

**24 - Metal Mode** calls `metalMode()` - no sync

**28 - Ripple Geddon** calls `rippleGeddon()` - no sync

**29 - Tail Time** calls `tailTime()` - no sync

**30 - Rainbow Noise** sets palette 9, calls `noiseTest()` then `noiseFader()` - no sync

**31 - Flash Grid** calls `flashGrid()` - no sync

**39 - Rainbow Spiral** calls `rainbowSpiral()` - no sync

**40 - Blender** calls `blender()` - no sync

**51 - MG Noise Party** calls `mg_noise_party()` - no sync

**52 - MG Blob** calls `mg_blob()` - no sync

**53 - MG Random** calls `mg_random()` - no sync

**54 - Enlightenment Achieved** calls `enlightenmentAchieved()` - **SYNCHRONIZED**

**63 - Node Counter** calls `nodeCounter()` to flash LED count - no sync - debounced

---

### 50 - Zoom to Colour

**Purpose:** Activates zoom-to-color effect with auto-cycling palette.

**Parameters:** 1
- `val` (int): Palette index to zoom to

**Format:**
```json
{
  "msgType": 50,
  "val": 7,
  "startTime": 1234567890
}
```

**Synchronized:** YES - Uses `startTime` for coordinated effect

**Magic Button:** `..--.-` (short-short-long-long-short-long)

**Notes:** Calls `zoomToColour(val)`. When triggered by magic button, cycles palette first then uses new index. Special handling in `magicButtonHandler.h`.

---

## Configuration Commands (10-13, 16-19, 25-26, 32-34)

### 10 - Set Step Rate

**Purpose:** Sets animation step rate (speed).

**Parameters:** 1
- `val` (int): Step rate value (1-255)

**Format:**
```json
{
  "msgType": 10,
  "val": 50
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)
- Browser → Server → All devices

**Synchronized:** No

**Notes:** Calls `setStepRate(val)`. Lower values = slower animation. Magic button pattern `.-.-` (short-long-short-long) + value sequence.

---

### 11 - Set Fade Rate

**Purpose:** Sets fade rate for transitions.

**Parameters:** 1
- `val` (int): Fade rate value (1-255)

**Format:**
```json
{
  "msgType": 11,
  "val": 100
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)
- Browser → Server → All devices

**Synchronized:** No

**Notes:** Calls `setFadeRate(val)`. Higher values = faster fade. Magic button pattern `.-..` (short-long-short-short) + value sequence.

---

### 12 - Set Brightness

**Purpose:** Sets global LED brightness.

**Parameters:** 1
- `val` (int): Brightness value (0-255)

**Format:**
```json
{
  "msgType": 12,
  "val": 200
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)
- Browser → Server → All devices

**Synchronized:** No

**Notes:** Calls `FastLED.setBrightness(val)`. 0 = off, 255 = full brightness. Magic button pattern `..--` (short-short-long-long) + value sequence.

---

### 13 - Set Number of Ripples

**Purpose:** Sets number of concurrent ripples in ripple effects.

**Parameters:** 1
- `val` (int): Number of ripples (1-10)

**Format:**
```json
{
  "msgType": 13,
  "val": 5
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)
- Browser → Server → All devices

**Synchronized:** No

**Notes:** Calls `setNRipples(val)`. Magic button pattern `..--.` (short-short-long-long-short) + value sequence.

---

### 16 - Noise Length

**Purpose:** Sets length parameter for noise patterns.

**Parameters:** 1
- `val` (int): Noise length value

**Format:**
```json
{
  "msgType": 16,
  "val": 30
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** No

**Notes:** Sets `mainState.noise.plength`. Magic button pattern `.----` (short-long-long-long-long) + value sequence.

---

### 17 - Noise Speed

**Purpose:** Sets speed parameter for noise patterns.

**Parameters:** 1
- `val` (int): Noise speed value

**Format:**
```json
{
  "msgType": 17,
  "val": 20
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** No

**Notes:** Sets `mainState.noise.pspeed`. Magic button pattern `.---.` (short-long-long-long-short) + value sequence.

---

### 18 - Noise Fade Length

**Purpose:** Sets fade length for noise patterns.

**Parameters:** 1
- `val` (int): Noise fade length value

**Format:**
```json
{
  "msgType": 18,
  "val": 25
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** No

**Notes:** Sets `mainState.noiseFade.plength`. Magic button pattern `.--.-` (short-long-long-short-long) + value sequence.

---

### 19 - Noise Fade Speed

**Purpose:** Sets fade speed for noise patterns.

**Parameters:** 1
- `val` (int): Noise fade speed value

**Format:**
```json
{
  "msgType": 19,
  "val": 15
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** No

**Notes:** Sets `mainState.noiseFade.pspeed`. Magic button pattern `.--..` (short-long-long-short-short) + value sequence.

---

### 25 - Air Length

**Purpose:** Sets length parameter for air mode pattern.

**Parameters:** 1
- `val` (int): Air length value

**Format:**
```json
{
  "msgType": 25,
  "val": 40
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** No

**Notes:** Sets `mainState.air.plength`. Magic button pattern `..--.` (short-short-long-long-short) + value sequence.

---

### 26 - Air Speed

**Purpose:** Sets speed parameter for air mode pattern.

**Parameters:** 1
- `val` (int): Air speed value

**Format:**
```json
{
  "msgType": 26,
  "val": 10
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** No

**Notes:** Sets `mainState.air.pspeed`. Magic button pattern `..-.-` (short-short-long-short-long) + value sequence.

---

### 32 - Fire Decay

**Purpose:** Sets decay rate for fire mode pattern.

**Parameters:** 1
- `val` (int): Fire decay value

**Format:**
```json
{
  "msgType": 32,
  "val": 120
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** No

**Notes:** Sets `mainState.fire.decay`. Higher values = faster decay. Magic button pattern `.-----` (short-long-long-long-long-long) + value sequence.

---

### 33 - Fire Speed

**Purpose:** Sets speed parameter for fire mode pattern.

**Parameters:** 1
- `val` (int): Fire speed value

**Format:**
```json
{
  "msgType": 33,
  "val": 8
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** No

**Notes:** Sets `mainState.fire.pspeed`. Magic button pattern `.----.` (short-long-long-long-long-short) + value sequence.

---

### 34 - Set Palette

**Purpose:** Sets color palette by index.

**Parameters:** 1
- `val` (int): Palette index number

**Format:**
```json
{
  "msgType": 34,
  "val": 3
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** No

**Notes:** Calls `setPalette(val)`. Used to synchronize palette across devices. Magic button sends this after cycling palette locally. Magic button pattern `.---.-` (short-long-long-long-short-long) + value sequence.

---

### 48 - Sweep

**Purpose:** Activates sweep effect with specified visual mode flags.

**Parameters:** 1
- `val` (int): Bit flags for visual modes (plength parameter)

**Format:**
```json
{
  "msgType": 48,
  "val": 7
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** YES - Uses `startTime` for coordinated effect

**Notes:** Enables `mainState.sweep.enabled` and sets `mainState.sweep.plength`. The plength value uses bit flags to control visual modes in the sweep effect. Synchronized to ensure all devices sweep together. Magic button pattern `..----` (short-short-long-long-long-long) + value sequence.

---

## Advanced Pattern Commands (35-43)

### 35 - Set Custom Palette Array

**Purpose:** Sets a custom 16-color palette from RGB array.

**Parameters:** 1
- `palette` (array): 48-element array (16 colors × 3 RGB values)

**Format:**
```json
{
  "msgType": 35,
  "palette": [255, 0, 0, 0, 255, 0, 0, 0, 255, ...]
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)
- Browser → Server → All devices

**Synchronized:** No

**Notes:** Copies array to `paletteArray` and sets `targetPalette` with 16 CRGB colors. Used by browser UI color picker.

---

### 36 - Set Pattern Parameter

**Purpose:** Sets specific parameter of a pattern by pointer.

**Parameters:** 3
- `pointer` (int): Index into patternPointers array
- `param` (int): Parameter type (0=speed, 1=length, 2=decay)
- `val` (int): Value to set

**Format:**
```json
{
  "msgType": 36,
  "pointer": 2,
  "param": 0,
  "val": 50
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)
- Browser → Server → All devices

**Synchronized:** No

**Notes:** Advanced pattern control. If `powerSaver` is at level 6, triggers `holdingPatternMode(1)`.

---

### 37 - Enable Single Pattern

**Purpose:** Disables all patterns except the specified one.

**Parameters:** 1
- `pointer` (int): Index into patternPointers array to enable

**Format:**
```json
{
  "msgType": 37,
  "pointer": 5
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)
- Browser → Server → All devices

**Synchronized:** No

**Notes:** Calls `patternsOff()` then enables specified pattern. Used for isolating a single effect.

---

### 38 - Toggle Pattern State

**Purpose:** Enables or disables a specific pattern.

**Parameters:** 2
- `pointer` (int): Index into patternPointers array
- `enabled` (bool): Enable (true) or disable (false)

**Format:**
```json
{
  "msgType": 38,
  "pointer": 3,
  "enabled": true
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)
- Browser → Server → All devices

**Synchronized:** No

**Notes:** Toggles pattern on/off without affecting other patterns. Useful for layering effects.

---

### 41 - Enlightenment Callback

**Purpose:** Triggers enlightenment effect with specific value.

**Parameters:** 1
- `val` (int): Enlightenment intensity/value

**Format:**
```json
{
  "msgType": 41,
  "val": 75
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** No

**Notes:** Calls `enlightenmentCallback(val)`. Magic button pattern `.-.--.` (short-long-short-long-long-short) + value sequence.

---

### 42 - Skater Direction

**Purpose:** Controls skater effect direction and magnitude.

**Parameters:** 2
- `magnitude` (int): Speed/intensity (0 = disable)
- `direction` (int): Direction selector (1 or other)

**Format:**
```json
{
  "msgType": 42,
  "magnitude": 5,
  "direction": 1
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)
- Browser joystick → Server → All devices

**Synchronized:** No

**Notes:** If magnitude is 0, disables skaters. Otherwise enables with specified speed and direction. Updates `stripDirection` array based on `directionLR` or `directionUD`.

---

### 43 - Launch Controller

**Purpose:** Controls launch sequence state from browser.

**Parameters:** 2
- `enabled` (bool): Launch system enabled
- `primed` (bool): Launch ready to fire

**Format:**
```json
{
  "msgType": 43,
  "enabled": true,
  "primed": false
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Browser → Server → All devices

**Synchronized:** No

**Notes:** If not enabled, calls `upset_mainState()`. If primed, enables launch. Otherwise calls `tripperTrapMode()`.

---

## System Control (44-46, 55)

### 44 - Lock State

**Purpose:** Controls parameter locks to prevent changes.

**Parameters:** 1
- `locks` (array): Array of lock objects with `lockNumber` and `lockState`

**Format:**
```json
{
  "msgType": 44,
  "locks": [
    {"lockNumber": 0, "lockState": true},
    {"lockNumber": 2, "lockState": false}
  ]
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)
- Server (`server/app.js`) - stores state
- Browser (`data/main.js`) - displays UI

**Network Flow:**
- Browser → Server (query) → Browser (response)
- Browser → Server → All devices (update)

**Synchronized:** No

**Notes:** Lock numbers: 0=stepRate, 1=fadeRate, 2=brightness, 3=palette. Server maintains lock state. Browser can query with "lockState" string message.

---

### 45 - Disable Bored Timer

**Purpose:** Stops server's automatic random pattern generation.

**Parameters:** 0

**Format:**
```json
{
  "msgType": 45
}
```

**Parsed By:**
- Server (`server/app.js`)

**Network Flow:**
- Browser → Server only (not broadcast)

**Synchronized:** No

**Notes:** Clears the bored timer interval. Server normally sends random patterns every 7 minutes when idle.

---

### 46 - Enable Bored Timer

**Purpose:** Starts/restarts server's automatic random pattern generation.

**Parameters:** 0

**Format:**
```json
{
  "msgType": 46
}
```

**Parsed By:**
- Server (`server/app.js`)

**Network Flow:**
- Browser → Server only (not broadcast)

**Synchronized:** No

**Notes:** Sets bored timer interval to 7 minutes. Timer also resets on any received message.

---

### 55 - Start Return Timer

**Purpose:** Starts the return timer.

**Parameters:** 0

**Format:**
```json
{
  "msgType": 55
}
```

**Parsed By:**
- ESP devices (`keeperOfTheDiamondLightCode.ino` - processWSMessage)

**Network Flow:**
- Server → All devices (broadcast)

**Synchronized:** No

**Notes:** Calls `returnTimer.startTimer()`. Magic button pattern `..-...` (short-short-long-short-short-short).

---

## Message Flow Diagrams

### Device Connection Flow
```
Client connects to server
    ↓
Client sends Role (997)
    ↓
Server assigns to role list
    ↓
If pyramid: Server sends Sweep Spot (993)
    ↓
Server broadcasts Total LED Count (994)
    ↓
All clients update and flash
```

### Time Synchronization Flow
```
Client sends Ping (999) with t0
    ↓
Server receives, captures time
    ↓
Server sends Pong (998) with t0 + serverTime
    ↓
Client calculates RTT and offset
    ↓
Client can now use server time for sync
```

### Mesh Bridge Flow
```
Mesh nodes connect to bridge
    ↓
Bridge sends Node Count (995) to server
    ↓
Server calculates total LED count
    ↓
Server sends Total LED Count (994) to bridge
    ↓
Bridge extracts nonMeshCount
    ↓
Bridge announces Bridge Announce (996) to mesh
    ↓
Mesh nodes calculate sweepSpot
```

### Synchronized Effect Flow (Launch, Zoom, etc.)
```
Browser/Magic Button triggers effect
    ↓
Message sent to server
    ↓
Server adds startTime = now() + delay
    ↓
Server broadcasts to all clients
    ↓
Each client schedules effect at startTime
    ↓
All clients execute simultaneously
```

---

## Programming Guidelines

### Adding New Message Types

1. **Choose a number**: Check this document for available numbers
2. **Add to ESP code**:
   - Add case to `processWSMessage()` in `.ino` file
   - If 0 parameters, add to `magicButtonHandler.h` parameter list
   - Add to `getCommandName()` switch in `magicButtonHandler.h`
3. **Add to server** (if special handling needed):
   - Add handling in `server/app.js` message handler
4. **Add to browser** (if web UI control):
   - Add control in `data/index.html`
   - Add handler in `data/main.js`
5. **Update documentation**: Add entry to this file

### Synchronized Effects

To make an effect synchronized:
1. Check for `startTime` in message
2. Store message and `startTime` in `meshMsgString` and `pendingStartTime`
3. In main loop, execute when `serverTime()` >= `pendingStartTime`
4. Add message type to `needsSync()` in `magicButtonHandler.h`

### Magic Button Patterns

Magic button sequences are binary (short=1, long=0) read MSB first:
- `.` = 1
- `.-` = 2
- `..` = 3
- `.--` = 4
- etc.

Update `magicCodes.txt` when adding new patterns.

---

## Message Type Quick Reference Table

| Type | Name | Parameters | Synchronized | Magic Button |
|------|------|------------|--------------|--------------|
| 999 | PING | 1 | No | - |
| 998 | PONG | 2 | No | - |
| 997 | ROLE | 1 | No | - |
| 996 | BRIDGE_ANNOUNCE | 1 | No | - |
| 995 | NODE_COUNT | 1 | No | - |
| 994 | TOTAL_LED_COUNT | 3 | No | - |
| 993 | SWEEP_SPOT | 1 | No | - |
| 1 | Upset MainState | 0 | No | `.` |
| 2 | Double Rainbow | 0 | No | `.-` |
| 3 | Tranquility Mode | 0 | No | `..` |
| 4 | Next Palette | 0 | No | `.--` |
| 5 | Tripper Trap Mode | 0 | No | `.-.` |
| 6 | Ants Mode | 0 | No | `..-` |
| 7 | Shooting Stars | 0 | No | `...` |
| 8 | Toggle House Lights | 0 | No | `.---` |
| 9 | Launch | 0 | YES | `.--.` |
| 10 | Set Step Rate | 1 | No | `.-.-` |
| 11 | Set Fade Rate | 1 | No | `.-..` |
| 12 | Set Brightness | 1 | No | `..--` |
| 13 | Set N Ripples | 1 | No | `..--.` |
| 14 | Noise Test | 0 | No | `...-` |
| 15 | Noise Fader | 0 | No | `....` |
| 16 | Noise Length | 1 | No | `.----` |
| 17 | Noise Speed | 1 | No | `.---.` |
| 18 | Noise Fade Length | 1 | No | `.--.-` |
| 19 | Noise Fade Speed | 1 | No | `.--..` |
| 20 | Earth Mode | 0 | No | `.-.--` |
| 21 | Fire Mode | 0 | No | `.-.-.` |
| 22 | Air Mode | 0 | No | `.-..-` |
| 23 | Water Mode | 0 | No | `.-...` |
| 24 | Metal Mode | 0 | No | `..---` |
| 25 | Air Length | 1 | No | `..--.` |
| 26 | Air Speed | 1 | No | `..-.-` |
| 28 | Ripple Geddon | 0 | No | `...--` |
| 29 | Tail Time | 0 | No | `...-.` |
| 30 | Rainbow Noise | 0 | No | `....-` |
| 31 | Flash Grid | 0 | No | `.....` |
| 32 | Fire Decay | 1 | No | `.-----` |
| 33 | Fire Speed | 1 | No | `.----.` |
| 34 | Set Palette | 1 | No | `.---.-` |
| 35 | Custom Palette Array | 1 | No | - |
| 36 | Set Pattern Parameter | 3 | No | - |
| 37 | Enable Single Pattern | 1 | No | - |
| 38 | Toggle Pattern State | 2 | No | - |
| 39 | Rainbow Spiral | 0 | No | `.--...` |
| 40 | Blender | 0 | No | `.-.---` |
| 41 | Enlightenment Callback | 1 | No | `.-.--.` |
| 42 | Skater Direction | 2 | No | - |
| 43 | Launch Controller | 2 | No | - |
| 44 | Lock State | 1 | No | - |
| 45 | Disable Bored Timer | 0 | No | - |
| 46 | Enable Bored Timer | 0 | No | - |
| 48 | Sweep | 1 | YES | `..----` |
| 50 | Zoom to Colour | 1 | YES | `..--.-` |
| 51 | MG Noise Party | 0 | No | `..--..` |
| 52 | MG Blob | 0 | No | `..-.--` |
| 53 | MG Random | 0 | No | `..-.-.` |
| 54 | Enlightenment Achieved | 0 | YES | `..-..-` |
| 55 | Start Return Timer | 0 | No | `..-...` |
| 63 | Node Counter | 0 | No | `......` |

---

## Version History

- **v1.0** (2026-04-05): Initial comprehensive documentation
  - Documented all message types 1-63 and 993-999
  - Added network flow diagrams
  - Added programming guidelines
  - Added quick reference table
