let peerConnection = null;
let signalConnection = null;

export function initializeWebRTC(videoElementId, sessionId) {
    const videoElement = document.getElementById(videoElementId);
    if (!videoElement) {
        throw new Error('Video element not found');
    }

    // Create WebRTC peer connection
    const configuration = {
        iceServers: [
            { urls: 'stun:stun.l.google.com:19302' }
        ]
    };

    peerConnection = new RTCPeerConnection(configuration);

    // Handle incoming streams
    peerConnection.ontrack = (event) => {
        if (event.streams && event.streams[0]) {
            videoElement.srcObject = event.streams[0];
        }
    };

    // Connect to SignalR hub
    signalConnection = new signalR.HubConnectionBuilder()
        .withUrl('/visionHub')
        .build();

    signalConnection.on('ReceiveOffer', async (offer) => {
        await peerConnection.setRemoteDescription(new RTCSessionDescription(offer));
        const answer = await peerConnection.createAnswer();
        await peerConnection.setLocalDescription(answer);
        await signalConnection.invoke('SendAnswer', sessionId, answer);
    });

    signalConnection.on('ReceiveIceCandidate', async (candidate) => {
        await peerConnection.addIceCandidate(new RTCIceCandidate(candidate));
    });

    // Handle ICE candidates
    peerConnection.onicecandidate = (event) => {
        if (event.candidate) {
            signalConnection.invoke('SendIceCandidate', sessionId, event.candidate);
        }
    };

    // Start the connection
    signalConnection.start().then(() => {
        signalConnection.invoke('JoinSession', sessionId);
    });
}

export function disposeWebRTC() {
    if (peerConnection) {
        peerConnection.close();
        peerConnection = null;
    }
    if (signalConnection) {
        signalConnection.stop();
        signalConnection = null;
    }
}