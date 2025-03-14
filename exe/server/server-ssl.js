const https = require('https');
const fs = require('fs');
const websocket = require('websocket');

const clientConnections = new Map(); // Map<clientid, connection>
const clientInfos = new Map();       // Map<clientid, levelid>

// Charger les certificats SSL
const options = {
    key: fs.readFileSync('../web-pem/private-key.pem'),
    cert: fs.readFileSync('../web-pem/certificate.pem')
};

const httpsServer = https.createServer(options, (req, res) => {
    const date = new Date().toUTCString().replace(' GMT', '');
    console.log(`[${date}] ${req.method.toUpperCase()} ${req.url}`);
    res.writeHead(404, { 'Content-Type': 'text/plain', 'Access-Control-Allow-Origin': '*' });
    res.end('Not Found');
});

const wsServer = new websocket.server({ httpServer: httpsServer });

wsServer.on('request', (req) => {
    const { path } = req.resourceURL;
    const splitted = path.split('/').slice(1);
    const clientid = splitted[0];
    const levelid = parseInt(splitted[1], 10) || -1;

    const connection = req.accept(null, req.origin);
    clientConnections.set(clientid, connection);
    clientInfos.set(clientid, levelid);

    const date = new Date().toUTCString().replace(' GMT', '');
    console.log(`[${date}] WebSocket request ${req.resourceURL} clientid=${clientid} levelid=${levelid}`);

    connection.on('message', (data) => {
        const date = new Date().toUTCString().replace(' GMT', '');
        if (data.type === 'utf8') {                  // receive an string message from a client
            const message = JSON.parse(data.utf8Data)
            const destId = message.id;
            const destConn = clientConnections.get(destId);
            console.log(`[${date}] Client ${clientid} << receive message = ${message}`);

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
            console.log(`[${date}] Client ${clientid} << receive binary data`);

            if (destPeerId.length > 1) {            // peer id : specific message for a peer
                const destConn = clientConnections.get(destPeerId);
                if (destConn) {
                    console.log(`... Transfer data to >> Client ${destPeerId}`);
                    destConn.sendBytes(data.binaryData);
                } else
                    console.error(`[${date}] Client ${destPeerId} not found`);
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
        const date = new Date().toUTCString().replace(' GMT', '');
        console.error(`[${date}] Client ${clientid} disconnected`);
        clientConnections.delete(clientid);
        clientInfos.delete(clientid);
        sendPeerInfos();
    });

    sendPeerInfos();
});

function sendPeerInfos() {
    const date = new Date().toUTCString().replace(' GMT', '');
    if (clientConnections.size > 0) {
        const data = JSON.stringify({ peers: Array.from(clientConnections.keys()), infos: Array.from(clientInfos.values())});

        for (const connection of clientConnections.values()) {
            connection.send(data);
        }

        console.log(`[${date}] WebSocket peersList Sent: ${data}`);
    } else {
        console.log(`[${date}] WebSocket empty peersList`);
    }
}

const hostname = process.env.IP || '127.0.0.1';
const port = process.env.PORT || '8100';

httpsServer.listen(port, hostname, () => {
    const date = new Date().toUTCString().replace(' GMT', '');
    console.log(`[${date}] Server listening on IP ${hostname} PORT ${port}`);
});

