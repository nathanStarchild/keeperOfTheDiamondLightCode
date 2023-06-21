const express = require('express');
const path = require('path');
const { createServer } = require('http');

const { WebSocketServer } = require('ws');
const app = express();
const port = 3000;

var hostName = 'antarean.pyramid';

const server = createServer(app);
const wss = new WebSocketServer({ server });

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

wss.on('connection', function connection(ws) {
    ws.on('error', console.error);
  
    ws.on('message', function message(data, isBinary) {
        console.log(`got a message: ${data}`)
        //broadcast the message
        wss.clients.forEach(function each(client) {
            if (client !== ws && client.readyState === WebSocket.OPEN) {
                client.send(data, { binary: isBinary });
            }
        });
    });
});

server.listen(port, () => {
    console.log(`Server listening on port ${port}`)
})