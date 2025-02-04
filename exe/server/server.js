
const http = require('http');
const websocket = require('websocket');

const clientConnections = new Map(); // Map<clientid, connection>
const clientInfos = new Map();       // Map<clientid, levelid>

const httpServer = http.createServer((req, res) => {
    console.log(`${req.method.toUpperCase()} ${req.url}`);
    res.writeHead(404, { 'Content-Type': 'text/plain', 'Access-Control-Allow-Origin': '*' });
    res.end('Not Found');
});

const wsServer = new websocket.server({httpServer});

wsServer.on('request', (req) => {

    const {path} = req.resourceURL;
    const splitted = path.split('/').slice(1);
    const clientid = splitted[0];
    const levelid = parseInt(splitted[1], 10) || -1;

    const connection = req.accept(null, req.origin);
    clientConnections.set(clientid, connection);
    clientInfos.set(clientid, levelid);

    console.log(`WebSocket request ${req.resourceURL} clientid=${clientid} levelid=${levelid}`);

    connection.on('message', (data) => {
        if (data.type === 'utf8') {                  // receive an string message from a client
            const message = JSON.parse(data.utf8Data)
            const destId = message.id;
            const destConn = clientConnections.get(destId);
            console.log(`Client ${clientid} << receive message = ${message}`);

            if (destConn) {                         // transfer the message to the destination client if exists
                message.id = clientid;
                destConn.send(JSON.stringify(message));
                console.log(`... Transfer message to >> Client ${destId}`);
            } else {
                console.error(`... Transfer message ... Client ${destId} not found`);
            }
        } else if (data.type === 'binary' && levelid !== -1) {  // receive a binary message
            const iDestPeerId = data.binaryData.indexOf(0, 0);
            const destPeerId  = data.binaryData.toString('utf-8', 0, iDestPeerId > 0 ? iDestPeerId : 0);
            console.log(`Client ${clientid} << receive binary data`);

            if (destPeerId.length > 1) {            // peer id : specific message for a peer
                const destConn = clientConnections.get(destPeerId);
                if (destConn) {
                    console.log(`... Transfer data to >> Client ${destPeerId}`);
                    destConn.sendBytes(data.binaryData);
                } else
                    console.error(`Client ${destPeerId} not found`);
            }
            else {                                  // broadcast message to clients with always excluding the sender
                const iSrcPeerId  = data.binaryData.indexOf(0, iDestPeerId > 0 ? iDestPeerId+1 : 0);
                const srcPeerId   = data.binaryData.toString('utf-8', iDestPeerId > 0 ? iDestPeerId+1 : 0, iSrcPeerId > 0 ? iSrcPeerId : 0);
                const iOrder      = data.binaryData.indexOf(0, iSrcPeerId > 0 ? iSrcPeerId+1 : 0);
                const order       = data.binaryData.toString('utf-8', iSrcPeerId > 0 ? iSrcPeerId+1 : 0 , iOrder > 0 ? iOrder : 0);
                if (order === 'needoffer') {
                    console.log(` ... Client ${clientid} receiver an offer from ${destPeerId}`);
                }
                console.log(` ... >> to all excluding ${srcPeerId} (client=${clientid} lvl=${levelid})`);

                const ids = clientConnections.keys();
                for (id of ids) {
                    if (id !== srcPeerId) {
                        const destConn = clientConnections.get(id);
                        const destInfo = clientInfos.get(id);
                        if (destInfo === levelid && destConn) { //
                            destConn.sendBytes(data.binaryData);
                        }
                    }
                }
            }
        }
    });

    connection.on('close', () => {
        console.error(`Client ${clientid} disconnected`);
        clientConnections.delete(clientid);
        clientInfos.delete(clientid);
        sendPeerInfos();
    });

    sendPeerInfos();
});


function sendPeerInfos() {
    if (clientConnections.size > 0) {
        const data = JSON.stringify({ peers: Array.from(clientConnections.keys()), infos: Array.from(clientInfos.values())});

        for (const connection of clientConnections.values()) {
            connection.send(data);
        }

        console.log(`WebSocket peersList Sent: ${data}`);
    } else {
        console.log(`WebSocket empty peersList`);
    }
}


const endpoint = process.env.PORT || '8080';
const splitted = endpoint.split(':');
const port = splitted.pop();
const hostname = splitted.join(':') || '127.0.0.1';

httpServer.listen(port, hostname, () =>
{
    console.log(`Server listening on ${hostname}:${port}`);
});
