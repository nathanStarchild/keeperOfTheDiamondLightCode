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

function broadcast(data, isBinary, from) {
    wss.clients.forEach(function each(client) {
        if (client !== from && client.readyState === WebSocket.OPEN) {
            client.send(data, { binary: isBinary });
        }
    });
}

function dontGetBored(){ 
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
      broadcast(data, false, null)
}

boredTimer = setInterval(dontGetBored, 7 * 1000 * 60)

wss.on('connection', function connection(ws) {
    ws.on('error', console.error);
  
    ws.on('message', function message(data, isBinary) {
        console.log(`got a message: ${data}`)
        if (data == "lockState"){
            lockState.forEach((value, index) => {
                let dat = {
                    "msgType": 44,
                    "enabled": value,
                    "lockNumber": index
                }
                ws.send(JSON.stringify(dat))
            })
            return
        }

        //from here we assume the message is a json
        let dat = JSON.parse(data)
        if (dat.msgType == 44){
            lockState[dat.lockNumber] = dat.enabled
        } else if (dat.msgType == 45) {
          clearInterval(boredTimer);
        } else if (dat.msgType == 46) {
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