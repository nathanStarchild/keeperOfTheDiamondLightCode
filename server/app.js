const express = require('express');
const path = require('path');
const { createServer } = require('http');

const { WebSocketServer, WebSocket } = require('ws');
const app = express();
const port = 3000;

var hostName = 'antarean.pyramid';

const server = createServer(app);
const wss = new WebSocketServer({ server });

let boredTimer;

// app.use((req, res, next) => {
//     if (req.get('host') != hostName) {
//         return res.redirect(`http://${hostName}`);
//     }
//     next();
// })

app.use(express.static(path.join(__dirname, '..', '/data')));

// app.get('/', (req, res, next) => {
//     res.send('Pi WiFi - Captive Portal');
// })

let lockState = [
    true,
    true,
    true,
    true,
]
lockState.forEach((value, index) => {
    let dat = {
        "msgType": 44,
        "enabled": value,
        "lockNumber": index
    }
    console.log(JSON.stringify(dat))
})

async function broadcastNew(data, isBinary, from) {
  const clients = [...wss.clients]

  for (let i = 0; i < clients.length; i++) {
    const client = clients[i]

    if (client !== from && client.readyState === WebSocket.OPEN) {
      client.send(data, { binary: isBinary })
      console.log(`sending to ${client}`)
    }

    if (i % 8 === 0) await new Promise(r => setImmediate(r))
  }
}

function broadcast(data, isBinary, from) {
    let sentCount = 0;
    let totalClients = wss.clients.size;
    console.log(`\n=== BROADCAST FUNCTION CALLED ===`);
    console.log(`Total clients in wss.clients: ${totalClients}`);
    console.log(`Data to send: ${data}`);
    console.log(`From client: ${from ? 'yes' : 'none'}`);
    
    wss.clients.forEach(function each(client, index) {
        const clientInfo = {
            role: client.role || 'none',
            readyState: client.readyState,
            readyStateName: ['CONNECTING', 'OPEN', 'CLOSING', 'CLOSED'][client.readyState],
            remoteAddress: client._socket ? client._socket.remoteAddress : 'N/A',
            remotePort: client._socket ? client._socket.remotePort : 'N/A',
            isSender: client === from
        };
        
        console.log(`\nClient ${index + 1}/${totalClients}:`, clientInfo);
        
        // if (client !== from && client.readyState === WebSocket.OPEN) {
        if (client.readyState === WebSocket.OPEN) {
            try {
                client.send(data, { binary: false });
                sentCount++;
                console.log(`  ✓ Successfully sent to this client`);
            } catch (err) {
                console.log(`  ✗ Error sending to this client:`, err.message);
            }
        } else {
            console.log(`  ✗ Skipped (readyState: ${clientInfo.readyStateName})`);
        }
    });
    console.log(`\nBroadcast complete: ${sentCount}/${totalClients} clients received message`);
    console.log(`=================================\n`);
}

async function sendToRole(data, isBinary, role) {
  const clients = [...wss.clients]
  let count = 0

  for (let i = 0; i < clients.length; i++) {
    const client = clients[i]

    if (client !== from && client.readyState === WebSocket.OPEN && client.role === role) {
      client.send(data, { binary: isBinary })
      count += 1
    }

    if (count % 8 === 0) await new Promise(r => setImmediate(r))
  }
}

function dontGetBored(){ 
  console.log("bored")
    let ran = Math.random();
    let msgType = 0;
    if (ran < 5/256) {
        msgType = 9
        //launch
      } else if (ran < 25/256) {
        msgType = 14
        //noise
      } else if (ran < 40/256) {
        msgType = 20
        // Serial.println("earth");
      } else if (ran < 60/256) {
        msgType = 21
        // Serial.println("fire");
      } else if (ran < 80/256) {
        msgType = 22
        // Serial.println("air");
      } else if (ran < 100/256) {
        msgType = 39
        // Serial.println("spiral");
      } else if (ran < 120/256) {
        msgType = 24
        // Serial.println("metal");
      } else if (ran < 140/256) {
        msgType = 5
        // Serial.println("tripperTrap");
      } else if (ran < 160/256) {
        msgType = 6
        // Serial.println("Ants!");
      } else if (ran < 180/256) {
        msgType = 2
        // Serial.println("rainbow");
      } else if (ran < 200/256) {
        msgType = 40
        // Serial.println("blender");
      } else if (ran < 220/256) {
        msgType = 3
        // Serial.println("tranquility");
      } else {
        msgType = 1
        // Serial.println("random");
      }
      let dat = {
          "msgType": msgType,
      }
      broadcast(JSON.stringify(dat), false, null)
}

boredTimer = setInterval(dontGetBored, 7 * 1000 * 60)

function sendPong(ws, t0){
  let dat = {
    "msgType": 998,
    "t0": t0,
    "serverTime": Date.now()
  }
  ws.send(JSON.stringify(dat))
}

wss.on('connection', function connection(ws) {
    console.log(`New client connected. Total clients: ${wss.clients.size}`);
    console.log(`Client info:`, {
        protocol: ws.protocol,
        readyState: ws.readyState,
        url: ws.url,
        headers: ws.upgradeReq ? ws.upgradeReq.headers : 'N/A',
        remoteAddress: ws._socket ? ws._socket.remoteAddress : 'N/A',
        remotePort: ws._socket ? ws._socket.remotePort : 'N/A'
    });
    
    ws.on('error', console.error);
  
    ws.on('message', function message(data, isBinary) {
        console.log(`\n=== Received message ===`);
        console.log(`From: ${ws.role || 'unknown role'}`);
        console.log(`isBinary: ${isBinary}`);
        console.log(`Data: ${data}`);
        console.log(`========================\n`);
        
        if (data == "lockState") {
            // Build an array of lock objects
            const locks = lockState.map((value, index) => ({
                lockNumber: index,
                lockState: value
            }));

            // Send a single message containing all locks
            const msg = {
                msgType: 44,
                locks: locks
            };

            ws.send(JSON.stringify(msg));
            return
        }

        //from here we assume the message is a json
        let dat = JSON.parse(data)
        //ping
        if (dat.msgType == 999 && typeof dat.t0 !== 'undefined'){
          sendPong(ws, dat.t0)
          return
        }
        // if (dat.msgType == 997) {
        //   ws.role = dat.role
        //   return
        // }
        if (dat.msgType == 44){
            lockState[dat.lockNumber] = dat.enabled
        } else if (dat.msgType == 45) {
          console.log("get bored")
          clearInterval(boredTimer);
        } else if (dat.msgType == 46) {
          console.log("don't get bored")
          clearInterval(boredTimer);
          boredTimer = setInterval(dontGetBored, 7 * 1000 * 60)
        } else {
          clearInterval(boredTimer);
          boredTimer = setInterval(dontGetBored, 7 * 1000 * 60)
        }

        //broadcast the message
        console.log(`\n=== About to broadcast ===`);
        console.log(`Original sender: ${ws.role || 'unknown'}`);
        console.log(`Message: ${data}`);
        console.log(`========================\n`);
        broadcast(data, isBinary, ws)
    });
    
    ws.on('close', function close() {
        console.log(`Client disconnected (role: ${ws.role || 'unknown'}). Total clients: ${wss.clients.size}`);
    });
});

server.listen(port, () => {
    console.log(`Server listening on port ${port}`)
})