
const http = require('http');
const websocket = require('websocket');

const clients = {};

const httpServer = http.createServer((req, res) =>
{
    console.log(`${req.method.toUpperCase()} ${req.url}`);

    const respond = (code, data, contentType = 'text/plain') =>
    {
        res.writeHead(code,
        {
'Content-Type' : contentType,
'Access-Control-Allow-Origin' : '*',
        });
        res.end(data);
    };

    respond(404, 'Not Found');
});

const wsServer = new websocket.server({httpServer});
wsServer.on('request', (req) =>
{
    console.log(`WebSocket request  ${req.resource}`);

    const {path} = req.resourceURL;
    const splitted = path.split('/');
    splitted.shift();
    const connectionId = splitted[0];

    const connection = req.accept(null, req.origin);
    connection.on('message', (data) =>
    {
        if (data.type === 'utf8')
        {
            console.log(`Client ${connectionId} << ${data.utf8Data}`);

            const message = JSON.parse(data.utf8Data);
            const destId = message.id;
            const dest = clients[destId];
            if (dest)
            {
                message.id = connectionId;
                const data = JSON.stringify(message);
                console.log(`Send >> Client ${destId}`);
                dest.send(data);
            }
            else
            {
                console.error(`Client ${destId} not found`);
            }
        }
        else if (data.type === 'binary')
        {
            console.log(`Client ${connectionId} << ${data.binaryData}`);

            const iDestPeerId = data.binaryData.indexOf(0, 0);
            const destPeerId  = data.binaryData.toString('utf-8', 0, iDestPeerId > 0 ? iDestPeerId : 0);
            if (destPeerId.length > 1)
            {
                // peer id : specific message for a peer

                const dest = clients[destPeerId];
                if (dest)
                {
                    console.log(`Send >> Client ${destPeerId}`);
                    dest.sendBytes(data.binaryData);
                }
                else
                {
                    console.error(`Client ${destPeerId} not found`);
                }
            }
            else
            {
                const iSrcPeerId  = data.binaryData.indexOf(0, iDestPeerId > 0 ? iDestPeerId+1 : 0);
                const srcPeerId   = data.binaryData.toString('utf-8', iDestPeerId > 0 ? iDestPeerId+1 : 0, iSrcPeerId > 0 ? iSrcPeerId : 0);
                const iOrder      = data.binaryData.indexOf(0, iSrcPeerId > 0 ? iSrcPeerId+1 : 0);
                const order       = data.binaryData.toString('utf-8', iSrcPeerId > 0 ? iSrcPeerId+1 : 0 , iOrder > 0 ? iOrder : 0);
                if (order === 'needoffer')
                {
                    console.log(`needoffer from ${destPeerId}`);

                }

                // broadcast message to all clients excluding the sender

                console.log(`Send >> to all excluding ${srcPeerId}`);

                const ids = Object.keys(clients);
                for (id of ids)
                {
                    if (id !== srcPeerId)
                        clients[id].sendBytes(data.binaryData);
                }
            }
        }
    });
    connection.on('close', () =>
    {
        delete clients[connectionId];
        console.error(`Client ${connectionId} disconnected`);

        sendAvailablePeers();
    });

    clients[connectionId] = connection;

    sendAvailablePeers();
});

function sendAvailablePeers()
{
    const ids = Object.keys(clients);
    if (ids.length)
    {
        // send the updated list of the available peers to all
        const connections = Object.values(clients);
        const data = JSON.stringify({ join: `${ids.toString()}` });
        for (const connection of connections)
            connection.send(data);

        console.log(`WebSocket peersList Sent ${data} !`);
    }
};


const endpoint = process.env.PORT || '8080';
const splitted = endpoint.split(':');
const port = splitted.pop();
const hostname = splitted.join(':') || '127.0.0.1';

httpServer.listen(port, hostname, () =>
{
    console.log(`Server listening on ${hostname}:${port}`);
});
