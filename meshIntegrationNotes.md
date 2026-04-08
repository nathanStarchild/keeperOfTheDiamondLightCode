## Option 3: Multi-Bridge Architecture Deep Dive

## Your Festival Setup - No Internet/Router

**Important Context:**
- Pi runs hostapd as the ONLY access point
- Pi serves web app to participants' phones/tablets (no internet)
- Pi runs WebSocket server for installation ESPs
- Installation ESPs need to forward messages to costume ESPs via mesh
- **No external router or internet connection**

### Network Architecture

```
Participants (phones/tablets)
        ↓ WiFi (channel X)
  [Raspberry Pi 4]
   - hostapd: Access Point "queerBurners" (channel X)
   - Node.js: WebSocket server (192.168.0.10:80)
   - HTTP server: Web app (data/index.html)
        ↓ WiFi Station (same channel X)
[Installation ESP32s] - DUAL MODE (STA + AP)
   - Station: Connected to Pi AP
   - AP: Mesh network node (BRIDGES)
        ↓ Mesh (same channel X)
[Costume ESP8266s/ESP32s]
   - Mesh-only when out of range
   - Can optionally connect to Pi when in range
```

### Critical: Channel Matching

**ESP8266 REQUIRES the same WiFi channel for STA and AP modes.**
**ESP32 is more flexible but still benefits from channel matching.**

You MUST configure:
1. Pi hostapd to use fixed channel (e.g., 6)
2. painlessMesh to use the same channel
3. Both in meshNode.h code

## Your Festival Setup - Channel Configuration

### Step 1: Configure Pi hostapd for Fixed Channel

Edit `/etc/hostapd/hostapd.conf` on your Raspberry Pi:

```conf
interface=wlan0
driver=nl80211
ssid=queerBurners
hw_mode=g
channel=6                    # ← CRITICAL: Fixed channel
ieee80211n=1
wmm_enabled=1
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=nextyearwasbetter
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
```

**Check current channel:**
```bash
iwlist wlan0 channel
# Find what channel your Pi AP is using
```

**After editing, restart hostapd:**
```bash
sudo systemctl restart hostapd
sudo systemctl status hostapd  # Verify it started correctly
```

### Step 2: Update meshNode.h to Match

In [meshNode.h](meshNode.h), the channel is now set:
```cpp
#define MESH_CHANNEL 6  // MUST match Pi hostapd channel!
```

If you change Pi to channel 11, change this to 11.

### Step 3: Device Strategy

**Installation ESPs (Bridges - Recommend ESP32):**
- Compile with `meshNode.h`
- Will connect to Pi via Station mode
- Will run mesh via AP mode
- Act as bridges (WebSocket → Mesh)
- **ESP32 strongly recommended** (more reliable dual-mode)

**Costume ESPs:**
- **Option A (Simplest)**: Mesh-only
  - Never connects to Pi
  - Always on mesh network
  - Receives forwarded messages from installation bridges
  - Works reliably on ESP8266 and ESP32
  
- **Option B (Advanced)**: Dual-mode like installation
  - Connects to Pi when in range
  - Falls back to mesh when out of range
  - **ESP32 only** - ESP8266 may have issues
  - More complex but provides direct Pi connection when close

**Recommendation for your 10 costumes:**
- If ESP8266: Use Option A (mesh-only)
- If ESP32: Can try Option B (dual-mode)

### Network Topology

### Network Topology

```
                    [Raspberry Pi AP]
                   192.168.0.10 (WebSocket Server)
                            |
        WiFi Station connections (when in range)
                            |
    +-----------------------+-----------------------+
    |                       |                       |
[Install ESP32 #1]   [Install ESP32 #2]    [Costume ESP8266 #1]
    |                       |                       |
    +----------- Mesh Network (Always Active) ------+
                            |
                    [Costume ESP8266 #2-10]
```

### How It Works

**Message Flow - Inbound (from Pi):**
1. Pi broadcasts WebSocket message to all connected devices
2. ESP receives via WebSocket → processes locally AND forwards to mesh
3. Mesh propagates message to all mesh nodes (including those out of Pi range)
4. Each node deduplicates (checks message ID to avoid loops)

**Message Flow - Outbound (to Pi):**
1. Costume sends mesh message with destination flag "toServer"
2. Any mesh node with Pi connection receives it → forwards via WebSocket
3. Pi receives and responds → cycle repeats

**Costume-to-Costume Sync:**
- All costumes constantly mesh-connected
- Time sync happens via mesh (painlessMesh has built-in time sync)
- Pattern/state messages propagate through mesh regardless of Pi connection

### painlessMesh Integration Points

**Key Features You'll Use:**
```cpp
mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
mesh.setRoot(false); // All devices can participate
mesh.stationManual(SSID, PASSWORD); // Connect to Pi AP in background
mesh.setContainsRoot(true); // Enables automatic bridge behavior
```

**Callbacks You'll Need:**
- `onReceive()` - Handle mesh messages (pattern changes, sync data)
- `onNewConnection()` - Track mesh topology changes
- `onChangedConnections()` - Update routing when nodes move
- `onNodeTimeAdjusted()` - Handle time sync (replaces your ping/pong)

### Coordination Between WebSocket and Mesh

**You'll need a message broker layer:**

```cpp
// Receives from BOTH sources
void handleIncomingMessage(JsonDocument& msg, MessageSource source) {
    // Process message for local LEDs
    processMessage(msg);
    
    // Forward if needed
    if (source == FROM_WEBSOCKET && mesh.isConnected()) {
        // Add message ID, forward to mesh
        forwardToMesh(msg);
    }
    else if (source == FROM_MESH && webSocket.isConnected()) {
        // Check if message wants to go to server
        if (msg["toServer"] == true) {
            forwardToWebSocket(msg);
        }
    }
}
```

### Technical Considerations

**Memory:**
- PainlessMesh adds ~15-20KB RAM overhead
- Each node maintains routing table (grows with mesh size)
- ESP8266 (costume): Tighter memory, but 10 nodes should be fine
- ESP32 (installation): Plenty of headroom

**Message Deduplication:**
Your current code tracks pending messages. You'll need:
```cpp
std::map<uint32_t, uint64_t> seenMessages; // messageID -> timestamp
// Prune old entries periodically to avoid memory growth
```

**Time Synchronization:**
- Your current offset-based sync (ping/pong) conflicts with mesh's time sync
- **Solution**: Use mesh time as master on costumes, sync mesh time to Pi time on installation nodes
- Installation nodes periodically inject Pi time into mesh

**WiFi Reconnection:**
- `mesh.stationManual()` handles Pi AP reconnection automatically
- PainlessMesh is resilient to station connection drops
- Mesh network continues functioning when Pi connection lost

### Critical Compatibility Issues

**SUMMARY**: Your current WiFi management code **directly conflicts** with painlessMesh and must be replaced.

**WiFi Event Handler Conflict:**
- **Problem**: Your current code subscribes to WiFi events (`WiFi.onStationModeGotIP`, `WiFi.onEvent`, etc.)
- **Issue**: painlessMesh also subscribes to the same WiFi events internally
- **Result**: Event handler collision will cause unpredictable behavior
- **Solution**: Remove your WiFi event handlers entirely, use painlessMesh callbacks instead

**Required Changes:**
```cpp
// REMOVE these:
#ifdef ESP8266
  stationConnectedHandler = WiFi.onStationModeGotIP(&WiFiGotIP);
  stationDisconnectedHandler = WiFi.onStationModeDisconnected(&WiFiStationDisconnected);
#else
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
#endif

// REPLACE with painlessMesh callbacks:
mesh.onChangedConnections([]() {
  // Handle connection/disconnection events
  // Check if station is connected: WiFi.status() == WL_CONNECTED
});
```

**WiFi Management:**
- painlessMesh uses native ESP32/ESP8266 SDK libraries (not Arduino WiFi libs)
- You don't need to call `WiFi.begin()`, `WiFi.disconnect()`, `WiFi.mode()` directly
- painlessMesh handles all WiFi initialization via `mesh.init()` and `mesh.stationManual()`
- Your WebSocket reconnection logic needs to adapt to mesh-managed WiFi

**Your wsSetup() function conflicts:**
```cpp
// REMOVE from wsSetup():
WiFi.disconnect();
WiFi.mode(WIFI_STA);              // painlessMesh needs WIFI_AP_STA mode!
WiFi.setAutoReconnect(false);
WiFi.setSleep(false);
WiFi.begin(SSID, WIFI_KEY);       // Use mesh.stationManual() instead

// REPLACE with:
mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
mesh.stationManual(SSID, WIFI_KEY);  // Handles Pi AP connection
mesh.setContainsRoot(true);
```

**Settings You Can Keep:**
- `WiFi.setSleep(false)` - Call AFTER mesh.init() if needed
- ArduinoOTA - Compatible with painlessMesh, keep as-is

**WebSocket Reconnection Logic:**
Your current code likely uses WiFi event handlers to trigger WebSocket reconnection. With painlessMesh:
```cpp
// In your loop() or scheduler:
void checkWebSocketConnection() {
  if (WiFi.status() == WL_CONNECTED && !webSocket.isConnected()) {
    // Pi AP is connected, but WebSocket is not
    webSocket.begin(WS_SERVER, WS_PORT, WS_PATH);
  }
}

// Or use mesh.onChangedConnections() callback:
mesh.onChangedConnections([]() {
  if (WiFi.status() == WL_CONNECTED) {
    // Station connected to Pi, start WebSocket if needed
    checkWebSocketConnection();
  } else {
    // Station disconnected, rely on mesh only
    webSocket.disconnect();
  }
});
```

### Specific Implementation Strategy

**Phase 1: Dual Protocol Support**
- Keep existing WebSocket code
- Add painlessMesh alongside
- Create message translation layer
- **Remove WiFi event handlers** (use mesh callbacks instead)

**Phase 2: Message Types**
You'll want to differentiate:
- `LOCAL_PATTERN` - Updates for LEDs (broadcast everywhere)
- `TIME_SYNC` - Pi time injected by bridge nodes
- `MESH_ONLY` - Costume coordination (don't send to Pi)
- `TO_SERVER` - Replies/status going back to Pi

**Phase 3: Conflict Resolution**
When costume receives same message from WebSocket AND mesh:
- Use message ID + timestamp to detect duplicates
- Process once, ignore duplicate
- Short-term cache (30 seconds) to track seen messages

### ESP8266 vs ESP32 Differences

**ESP8266 Costumes:**
- Slightly slower mesh participation
- More memory-constrained
- May need to reduce mesh message queue size
- Consider simpler pattern storage

**ESP32 Installation:**
- Can handle more complex routing logic
- Better as primary bridges due to dual-core
- Run WebSocket and mesh on separate cores if needed

### Potential Issues & Solutions

**Issue 1: Message Storms**
- Problem: 10+ devices all forwarding same Pi message
- Solution: Only devices with WebSocket connection forward, add small random delay

**Issue 2: Split-Brain Scenario**
- Problem: Some costumes see different Pi state due to mesh delays
- Solution: Timestamp messages, nodes accept newer state

**Issue 3: Mesh Formation Time**
- Problem: New device takes 10-30 seconds to join mesh
- Solution: Show connection status on LEDs, graceful degradation

**Issue 4: Desert Interference**
- Problem: 2.4GHz crowded at festival
- Solution: PainlessMesh allows channel configuration, scan for clearest channel at startup

### Performance Expectations

**Latency:**
- Direct WebSocket: ~20-50ms
- 1-hop mesh: +50-100ms
- 2-hop mesh: +100-200ms
- Acceptable for LED pattern sync

**Range:**
- ESP32: ~50-100m open field (desert)
- ESP8266: ~30-70m
- Mesh extends this significantly with multiple hops

Does this architecture make sense for your installation? Any specific concerns about the ESP8266 memory constraints or the message broker complexity?

---

## Implementation: meshNode.h

The `meshNode.h` file has been created implementing the Option 3 multi-bridge architecture.

### Installation Requirements

**Library Dependencies:**
```bash
# Install via Arduino Library Manager or PlatformIO:
- painlessMesh (latest version)
- ArduinoJson (v6.x)
- WebSockets (by Markus Sattler)
- TaskScheduler (required by painlessMesh)
- AsyncTCP (ESP32) or ESPAsyncTCP (ESP8266)
```

**PlatformIO (platformio.ini):**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    fastled/FastLED
    bblanchon/ArduinoJson @ ^6.21.0
    links2004/WebSockets @ ^2.3.6
    painlessmesh/painlessMesh @ ^1.5.0

[env:d1_mini]
platform = espressif8266
board = d1_mini
framework = arduino
lib_deps = 
    fastled/FastLED
    bblanchon/ArduinoJson @ ^6.21.0
    links2004/WebSockets @ ^2.3.6
    painlessmesh/painlessMesh @ ^1.5.0
```

### Important Notes

**Compatibility with Existing Code:**
- `processWSMessage()` function remains unchanged in your .ino file
- All your existing pattern functions work as-is
- `alone` variable: Now means "not connected to Pi" (but may be on mesh)
- `wsMsgString` and `wsMsg`: Used by both WebSocket and mesh messages

**Behavioral Differences:**
- **Time reference**: When on mesh only, uses `mesh.getNodeTime()` (microseconds)
- **Connection status**: Device can be "alone" (no Pi) but still connected via mesh
- **Message latency**: Mesh adds 50-200ms per hop vs direct WebSocket

### Usage

**In your main .ino file:**

```cpp
// At the top of keeperOfTheDiamondLightCode.ino, replace:
// #include "client.h"

// With conditional include:
//#define USE_MESH  // Uncomment for mesh-enabled devices

#ifdef USE_MESH
  #include "meshNode.h"
#else
  #include "client.h"
#endif
```

**Modify setup():**
```cpp
void setup() {
  Serial.begin(9600);
  Serial.println("hello");
  #ifdef ROLE
  Serial.println(ROLE);
  #endif
  
  elementSetup();
  
  // Replace wsSetup() with:
  #ifdef USE_MESH
    meshSetup();
  #else
    wsSetup();
  #endif
  
  // ... rest of setup ...
}
```

**Modify loop():**
```cpp
void loop() {
  // Replace wsLoop() with:
  #ifdef USE_MESH
    meshLoop();
  #else
    wsLoop();
  #endif
  
  // ... rest of loop ...
}
```

### Compile-Time Configuration

**Option 1: Simple Toggle**
Uncomment `#define USE_MESH` at top of .ino file for mesh devices.

**Option 2: Role-Based**
```cpp
// Automatically enable mesh for costumes
#if defined(ROLE) && (strcmp(ROLE, "costume") == 0)
  #define USE_MESH
#endif

#ifdef USE_MESH
  #include "meshNode.h"
#else
  #include "client.h"
#endif
```

**Option 3: Device-Specific**
```cpp
// Enable mesh for specific devices
#if defined(COSTUME_1) || defined(COSTUME_2) || defined(COSTUME_3)
  #define USE_MESH
#endif
```

Then compile with: `arduino-cli compile --fqbn esp8266:esp8266:d1_mini -D COSTUME_1 ...`

### Key Features Implemented

1. **Dual Protocol Support**: WebSocket client + painlessMesh running simultaneously
2. **Single Active Bridge with RSSI Election**: 
   - Only one device forwards WebSocket → Mesh at a time
   - Bridge election based on WiFi signal strength (RSSI) to Pi AP
   - Nodes with better signal become bridge
   - Nodes can challenge bridge if they have significantly better signal (+10 dBm advantage required)
   - Automatic failover: if bridge times out or loses AP connection, new bridge elected
   - Tie-breaker: Lower node ID wins if RSSI is equal
3. **Server-Based Message Deduplication**: 
   - Uses `msgId` from Pi server (one-to-one with messages)
   - Circular buffer tracks 20 most recent message IDs
   - 30-second time window for seen messages
4. **Time Synchronization**: Uses Pi time via ping/pong when connected, mesh time otherwise
5. **Pending Messages**: Queues messages with future startTime for synchronized execution
6. **Connection Resilience**: Continues operating on mesh when Pi AP unavailable
7. **ArduinoOTA**: Fully compatible, keeps your OTA update capability
8. **Status Tracking**: `alone` (not on Pi), `meshConnected`, `wsConnected`, `isBridge` flags
9. **Non-Blocking Architecture**: Minimal blocking (only 100ms during challenge response)

### Bridge Election and Message Flow

**How bridge election works:**

1. **Initial State**: No bridge exists, all nodes connected to Pi
   
2. **Node A** connects to Pi (RSSI: -45 dBm):
   - Checks for existing bridge → none found
   - Claims bridge role
   - Broadcasts "I am bridge, RSSI: -45"

3. **Node B** connects to Pi (RSSI: -60 dBm):
   - Hears Node A's announcement (RSSI: -45)
   - Compares: my -60 vs bridge -45
   - Node A has better signal → stays as non-bridge

4. **Node C** connects to Pi (RSSI: -35 dBm):
   - Hears Node A's announcement (RSSI: -45)
   - Compares: my -35 vs bridge -45
   - I have +10 dBm advantage → challenges bridge
   - Sends "Challenge: I have RSSI -35"
   - Node A receives challenge, sees worse signal, steps down
   - Node C becomes bridge

5. **Bridge announces every 5 seconds**:
   - "I am bridge %u, RSSI: %d"
   - All nodes update their understanding of current bridge
   - Nodes compare their signal, challenge if significantly better

6. **Bridge loses Pi connection**:
   - Immediately steps down
   - Stops announcing
   - After 20 seconds timeout, other nodes claim bridge role
   - Node with best signal (checking periodically) becomes new bridge

**Message forwarding flow:**

1. **Pi broadcasts message** with `msgId: 42` → all connected ESPs receive it
2. **Only the bridge** forwards to mesh (others ignore)
3. **Mesh propagates** message to all nodes
4. **Each node** checks `msgId: 42` → executes once

**Result:** 
- Single bridge = minimal mesh traffic
- Best signal quality for forwarding
- Automatic optimization as nodes move or signals change
- Each device executes message **exactly once**

### Configuration Constants

Edit in `meshNode.h` header:
- `MESH_SSID` / `MESH_PASSWORD`: Mesh network credentials
- `MESH_PORT`: Mesh communication port (default 5555)
- `MAX_SEEN_MESSAGES`: Deduplication cache size (default 20)
- `MAX_PENDING_MESSAGES`: Timed message queue size (default 2)

**Bridge election tuning**:
- `BRIDGE_TIMEOUT_US = 20000000`: Bridge timeout (20 seconds in microseconds)
- `BRIDGE_RSSI_ADVANTAGE = 10`: dBm advantage required to challenge bridge
- `bridgeAnnouncementTimer(5000)`: How often bridge announces (5 seconds)
- `bridgeCheckTimer(2000)`: How often non-bridges check for bridge (2 seconds)

**Signal strength interpretation:**
- RSSI values are negative (closer to 0 = better signal)
- `-30 dBm`: Excellent signal
- `-50 dBm`: Good signal  
- `-70 dBm`: Weak signal
- `-90 dBm`: Very poor signal

### Message Protocol

**Server Requirements (IMPORTANT):**
The Pi WebSocket server **must include a unique `msgId` field** in all broadcast messages:
```javascript
// Node.js example:
let messageCounter = 1;

function broadcastMessage(msgType, data) {
  const message = {
    msgId: messageCounter++,  // ← REQUIRED for mesh deduplication
    msgType: msgType,
    ...data
  };
  webSocketServer.broadcast(JSON.stringify(message));
}
```

Without server-side `msgId`, multiple bridge devices may create ID collisions.

**WebSocket → Mesh:**
- Automatic forwarding when mesh nodes connected
- Uses server's `msgId` field for deduplication (one-to-one mapping)
- Falls back to local ID generation only if server doesn't provide ID
- Small random delay (5-25ms) prevents message storms

**Mesh → WebSocket:**
- Include `"toServer": true` in message JSON
- Any bridge node will forward to Pi server
- Examples: status updates, user interactions from costumes

**Mesh-Only Messages:**
- Regular messages without `toServer` flag
- Stay within mesh network
- Useful for costume-to-costume coordination
- Generate local `msgId` using device counter

### Network Topology Detection

The code automatically detects:
- **Pi AP connection**: `WiFi.status() == WL_CONNECTED`
- **Mesh connectivity**: `mesh.getNodeList().size() > 0`
- **Bridge mode**: Both conditions true → forwards messages both ways
- **Isolated mode**: Only mesh → operates without Pi

### Memory Considerations

**ESP8266:**
- painlessMesh: ~15-20KB RAM overhead
- Seen messages: 20 × 16 bytes = 320 bytes
- Should work fine with 10 costume nodes

**ESP32:**
- Plenty of headroom
- Can increase `MAX_SEEN_MESSAGES` if needed

### Testing Checklist

1. **Mesh formation**: Upload to 2+ devices, verify mesh connects
2. **Pi connection**: Test with Pi AP available → WebSocket should connect
3. **Bridge forwarding**: Send WebSocket message → verify all mesh nodes receive
4. **Mesh messaging**: Pi off, verify mesh-only communication works
5. **Deduplication**: Verify no duplicate pattern execution
6. **Time sync**: Check offset values in serial output
7. **Reconnection**: Disconnect/reconnect Pi → verify recovery

### Troubleshooting

**Problem: Devices not joining mesh**
- Check `MESH_SSID` and `MESH_PASSWORD` match on all devices
- Verify `MESH_PORT` is the same (default 5555)
- Ensure devices are within range (~30-100m depending on hardware)
- Check Serial output for "Mesh: New connection" messages
- Try increasing mesh debug level: `mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION)`

**Problem: WebSocket not connecting when Pi available**
- Verify `SSID`, `WIFI_KEY`, and `WS_SERVER` IP are correct
- Check Pi AP is broadcasting and reachable
- Look for "WebSocket: Connected" in Serial output
- Confirm Pi web server is running on correct port (80)
- Try manual connection: `telnet 192.168.0.10 80`

**Problem: Duplicate messages being executed**
- **Most likely**: Server not sending `msgId` field (check server logs)
- Verify messages include `msgId` field in Serial output
- Look for "Warning - no server msgId" in device logs
- Check that `addMessageId()` function is working on server
- Increase `MAX_SEEN_MESSAGES` if you have many rapid messages
- Verify `pruneSeenMessages()` is running (every 30 seconds)
- Look for "Ignoring duplicate msgId" in Serial output

**Problem: No device becoming bridge**
- Check that at least one device is connected to Pi: WiFi.status() == WL_CONNECTED
- Look for "Bridge: No bridge detected, claiming role" messages
- Check mesh is connected: `mesh.getNodeList().size() > 0`
- Verify bridge protocol messages aren't being filtered
- Check that MSG_TYPE_BRIDGE_ANNOUNCE (990) isn't conflicting with other message types

**Problem: Wrong device is bridge (poor signal)**
- Wait for bridge announcements (every 5 seconds)
- Better-connected nodes should challenge after hearing announcement  
- Check RSSI values in logs: `WiFi.RSSI()`
- Verify `BRIDGE_RSSI_ADVANTAGE = 10` is appropriate for your environment
- Try reducing advantage threshold to 5 dBm for more aggressive challenges
- Manually trigger challenge by bringing better-connected node closer to AP

**Problem: Bridge keeps changing (flapping)**
- Check for unstable WiFi signal (nodes moving, interference)
- Increase `BRIDGE_RSSI_ADVANTAGE` to require larger signal difference
- Increase bridge announcement interval if network is stable
- Check for RSSI values near the threshold causing back-and-forth challenges

**Problem: Messages not forwarding to mesh**
- Check if device is bridge: Look for "Bridge: Announced" in logs
- If not bridge, check why: Look for "Bridge: Heard announce from X"
- Check mesh connection: `mesh.getNodeList().size()` should be > 0
- Verify no JSON parse errors in Serial output
- Ensure message size is under mesh limit (~1KB)
- Check "Forward: Sent msgId X to mesh" appears in bridge logs
- Non-bridge nodes should NOT forward (this is correct behavior)

**Problem: High memory usage on ESP8266**
- Reduce `MAX_SEEN_MESSAGES` (try 10 instead of 20)
- Reduce `MAX_PENDING_MESSAGES` if not using timed messages
- Disable mesh debug messages: `mesh.setDebugMsgTypes(ERROR)`
- Check free heap with `ESP.getFreeHeap()` in loop

**Problem: Mesh time sync issues**
- painlessMesh handles time sync automatically
- Allow 30-60 seconds for initial mesh time convergence
- If Pi time sync needed, ping/pong runs when WebSocket connected
- Check `offsetMs` value in Serial output for Pi time offset

**Problem: Patterns not syncing between devices**
- Verify all devices receiving messages (check Serial logs)
- Check `startTime` field in messages (must be future time)
- Ensure time sync is working (offsetMs should stabilize)
- Add delays between messages on Pi side (min 100ms)

### Serial Output Examples

**Successful mesh formation:**
```
meshNode: Initializing...
meshNode: Mesh ID = 3742831264
Mesh: New connection, nodeId = 2918463827
Mesh: Connections changed
Mesh: Nodes connected: 1
```

**Successful Pi + WebSocket connection:**
```
meshNode: Connected to Pi AP
meshNode: IP = 192.168.0.42
WebSocket: Connected to ws://192.168.0.10:80/
WebSocket: Valid JSON received: {"msgType":997}
```

**Bridge election process:**
```
# Node A claiming bridge (no existing bridge):
Bridge: No bridge detected, claiming role (RSSI: -45 dBm)
Bridge: Announced (RSSI: -45 dBm, nodes: 3)

# Node B hearing announcement (worse signal):
Bridge: Heard announce from 3742831264 (RSSI: -45 dBm)
(Node B remains non-bridge since its RSSI is -60 dBm)

# Node C challenging (better signal):
Bridge: Challenging bridge 3742831264 (my RSSI -35 > their -45)
Bridge: No response to challenge, claiming role
Bridge: Announced (RSSI: -35 dBm, nodes: 4)

# Node A stepping down:
Bridge: Heard announce from 2918463827 (RSSI: -35 dBm)
Bridge: Conflict! Node 2918463827 also claims bridge, checking signal...
Bridge: Stepping down (their RSSI -35 > my RSSI -45)
```

**Message forwarding (single bridge):**
```
# Bridge node:
WebSocket: Text: {"msgId":42,"msgType":5,"palette":3}
Forward: Sent msgId 42 to mesh (6 nodes)

# Non-bridge node (doesn't forward):
WebSocket: Text: {"msgId":42,"msgType":5,"palette":3}
(processes locally, doesn't forward to mesh)
```

**Mesh-only device receiving:**
```
Mesh: Received from 3742831264: {"msgId":42,"msgType":5,"palette":3}
meshNode: Executing message
```

### Next Steps

1. **Update Pi Server** (CRITICAL): Add `msgId` field to all broadcast messages
2. **Configure Pi hostapd** for fixed channel (e.g., channel 6)
3. **Update MESH_CHANNEL** in meshNode.h to match Pi channel
4. Install painlessMesh library and dependencies on ESP devices
5. **Test with installation ESP32s first:**
   - Compile 2 installation ESPs with meshNode.h
   - Verify both connect to Pi AP and mesh
   - Check which becomes bridge (better RSSI)
   - Send test message from web app
   - Verify bridge forwards to mesh
6. **Test costume strategy:**
   - If ESP8266: Create mesh-only version (remove WebSocket code)
   - If ESP32: Test with meshNode.h dual-mode
7. Test bridge handoff: disconnect bridge, verify new bridge elected
8. Test with device moving in/out of Pi range
9. Scale up to full costume fleet (10 devices)
10. Monitor memory usage on ESP8266 devices

### Testing Checklist for Your Setup

**Phase 1: Verify Pi Configuration**
- [ ] Pi hostapd configured with fixed channel
- [ ] Phones/tablets can connect to Pi AP
- [ ] Web app loads on phones (http://192.168.0.10 or similar)
- [ ] Installation ESPs can connect to Pi AP
- [ ] WebSocket connection works with client.h

**Phase 2: Add Mesh to Installation**
- [ ] Update 2 installation ESP32s with meshNode.h  
- [ ] Both form mesh network
- [ ] Both connect to Pi AP
- [ ] Check Serial: "Bridge: Announced" on one device
- [ ] Verify RSSI-based bridge election
- [ ] Send test message from web app
- [ ] Bridge device shows: "Forward: Sent msgId X to mesh"

**Phase 3: Add Costume ESPs**
- [ ] Decide: mesh-only (A) or dual-mode (B)
- [ ] Program 1 costume ESP with chosen approach
- [ ] Place out of Pi range
- [ ] Verify mesh connection to installation
- [ ] Send message from web app
- [ ] Costume receives and executes pattern
- [ ] Scale to all 10 costumes

**Phase 4: Festival Stress Test**
- [ ] All devices powered on
- [ ] Costumes walking around (in/out of range)
- [ ] Multiple participants using web app
- [ ] Verify patterns sync across all devices
- [ ] Check for dropped messages (monitor Serial)
- [ ] Verify mesh recovers from device disconnections

### Server-Side Implementation

Your Pi WebSocket server needs to include a unique `msgId` in every broadcast message.

**Implementation for your server/app.js:**

```javascript
// At the top of server/app.js, add message counter:
let messageIdCounter = 1;

// Create a wrapper function that adds msgId to all messages:
function addMessageId(data) {
  try {
    // Parse the message
    const message = typeof data === 'string' ? JSON.parse(data) : data;
    
    // Add msgId if not already present
    if (!message.msgId) {
      message.msgId = messageIdCounter++;
    }
    
    return JSON.stringify(message);
  } catch (err) {
    console.error('Error adding msgId:', err);
    return data;  // Return original if parsing fails
  }
}

// Modify your broadcast() function to add msgId:
function broadcast(data, isBinary, from) {
    let sentCount = 0;
    let totalClients = wss.clients.size;
    console.log(`\n=== BROADCAST FUNCTION CALLED ===`);
    console.log(`Total clients in wss.clients: ${totalClients}`);
    
    // Add msgId to the data
    const dataWithId = isBinary ? data : addMessageId(data);
    console.log(`Data to send: ${dataWithId}`);
    
    wss.clients.forEach(function each(client, index) {
        if (client.readyState === WebSocket.OPEN) {
            try {
                client.send(dataWithId, { binary: isBinary });
                sentCount++;
                console.log(`  ✓ Successfully sent to client ${index + 1}`);
            } catch (err) {
                console.log(`  ✗ Error sending to client:`, err.message);
            }
        }
    });
    console.log(`\nBroadcast complete: ${sentCount}/${totalClients} clients received message`);
    console.log(`=================================\n`);
}

// Also update broadcastNew() and sendToRole() similarly
async function broadcastNew(data, isBinary, from) {
  const clients = [...wss.clients];
  const dataWithId = isBinary ? data : addMessageId(data);

  for (let i = 0; i < clients.length; i++) {
    const client = clients[i];
    if (client !== from && client.readyState === WebSocket.OPEN) {
      client.send(dataWithId, { binary: isBinary });
    }
    if (i % 8 === 0) await new Promise(r => setImmediate(r));
  }
}
```

**Persistence across restarts (optional but recommended):**
```javascript
// Save counter periodically to survive server restarts
const fs = require('fs');

function loadMessageCounter() {
  try {
    return parseInt(fs.readFileSync('msgCounter.txt', 'utf8')) || 1;
  } catch {
    return 1;
  }
}

function saveMessageCounter() {
  fs.writeFileSync('msgCounter.txt', messageIdCounter.toString());
}

let messageIdCounter = loadMessageCounter();

// Save every 100 messages or on server shutdown
process.on('SIGINT', () => {
  saveMessageCounter();
  process.exit();
});
```

---

## Future Refactoring Ideas

### MagicButtonHandler: Array-Based Multi-Parameter Input

**Current Implementation:**
- Handler uses explicit states: WAITING_FOR_VALUE_1, WAITING_FOR_VALUE_2
- Supports up to 2 parameters per command
- Each parameter state handled separately in switch statement

**Future Enhancement:**
- Convert to array-based approach: `WAITING_FOR_VALUE` state with parameter index tracking
- Enable unlimited parameters per command (current commands only use 0-2, but architecture would scale)
- Cleaner code: single loop/logic handles all parameters
- Trade-off: ~50 lines of changes vs current ~10 line refactor for WAITING_FOR_DURATION removal

**Implementation Notes:**
- Would need: `uint8_t currentParamIndex` counter, `uint16_t paramValues[MAX_PARAMS]` array
- Single WAITING_FOR_VALUE state that increments currentParamIndex
- Loop through params checking inputType for each
- Deferred until needed - current 2-parameter maximum is sufficient for all existing commands

**Date Added:** 2026-04-08  
**Context:** After refactoring to remove WAITING_FOR_DURATION and use param.inputType checking
