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

async function broadcast(data, isBinary, from) {
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
    ws.on('error', console.error);
  
    ws.on('message', function message(data, isBinary) {
        console.log(`got a message: ${data}`)
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
        if (dat.msgType == 997) {
          ws.role = dat.role
          return
        }
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
        broadcast(data, isBinary, ws)
    });
});

server.listen(port, () => {
    console.log(`Server listening on port ${port}`)
})