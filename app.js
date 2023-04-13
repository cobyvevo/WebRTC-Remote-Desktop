const express = require("express");
const app = express();
const http = require("http");
const server = http.createServer(app);
const nodeWebRTC = require("wrtc");
const nodeWebRTCserver = require("wrtc").nonstandard;
const screengrabber = require("./build/Release/screengrabber")

const { Server } = require("socket.io");
const io = new Server(server);

const MouseEvents = {
	LEFT_DOWN: 0,
    LEFT_UP: 1,

    RIGHT_DOWN: 2,
    RIGHT_UP: 3,

    MIDDLE_DOWN: 4,
    MIDDLE_UP: 5,

    SCROLL: 6,

    //others

    KEYBOARD: 7,
    MOVEMENT: 8
}
	
let vars = {

	iceServers: [

      {
      	urls:[
				"stun:stun1.1.google.com:19302",
				"stun:stun2.1.google.com:19302",
				"stun:relay.metered.ca:80"
			]
      },

  ],

}

let refuseconnections = true
let REBOOT = false;

let peerConnection;
let ready_to_send_offer = false;
let connection_used = true;
let makeframes = false;

let inputChannel;
let videoSource;
let videoTrack;
let videoSink;
let transceiver;
let interval;

let scale = 20;

function pushFrame() {

	let settings = videoTrack.getConstraints
	
	if (makeframes == true) {

		let frame = screengrabber.GetFrame(scale); //get a screenshot from c++ code
		let width = screengrabber.GetWidth();
		let height = screengrabber.GetHeight();

		const i420Frame = {
	      width,
	      height,
	      data: new Uint8ClampedArray(1.5 * width * height)
	    };

	    const rgbframe = {
	      width,
	      height,
	      data: new Uint32Array(frame)
	    };

	    nodeWebRTCserver.rgbaToI420(rgbframe,i420Frame);

		videoSource.onFrame(i420Frame);
	}

}

function startFrameCap(fps) {
	if (interval) {
		clearInterval(interval);
	}
	interval = setInterval(pushFrame,1000/fps)
}

//WEBRTC
let bandwidth = 4096;
async function createOffer() {
	if (REBOOT == true) {
		return;
	}
	if (connection_used == false) {
		console.log("someone left, however the RTCPeerConnection wasnt used.");
		return;
	}
	console.log("new offer ...");

	ready_to_send_offer = false;
	connection_used = false;

	if (peerConnection) {
		console.log("closing connection...");
		clearInterval(interval);
		videoSink.stop();
		videoTrack.stop();
		peerConnection.close();
	}

	peerConnection = new nodeWebRTC.RTCPeerConnection(vars);

	inputChannel = peerConnection.createDataChannel("inputChannel");	

	videoSource = new nodeWebRTCserver.RTCVideoSource();

	videoTrack = videoSource.createTrack();
	transceiver = peerConnection.addTransceiver(videoTrack);
	videoSink = new nodeWebRTCserver.RTCVideoSink(transceiver.receiver.track);

	startFrameCap(30)

	peerConnection.addEventListener('connectionstatechange', (e) => {
		console.log(peerConnection.connectionstatechange);

	});

	peerConnection.onicecandidate = function(event) {
	  if (event.candidate) {
		 console.log(peerConnection.localDescription);
	    // Send the candidate to the remote peer
	  }
	};

	peerConnection.onicegatheringstatechange = function(event) {
	  console.log("ICE gathering state changed: " + event.target.iceGatheringState);

	  if (event.target.iceGatheringState == "complete") {
	  	console.log("Complete! ready to send.");
	  	ready_to_send_offer = true;
		io.emit("logmessage","all ICE candidates have been established!");		
		io.emit("RTCoffer",peerConnection.localDescription);
	  }
	};

	peerConnection.createOffer().then(offer => {	
		//offer.sdp = BandwidthHandler.setVideoBitrates(offer.sdp, {
		//			min: bandwidth,
		//			max: 4096
		//		})
		console.log(offer);
		peerConnection.setLocalDescription(offer);
	});

}

createOffer()

async function establishAnswer(answer) {
	if (!peerConnection.currentRemoteDescription) {
		connection_used = true;
		peerConnection.setRemoteDescription(answer);
		console.log("Success");
	}
}

let resetautoondisc = true;
let fulllock = false;

function reboot() {
	REBOOT = true
	console.log("rebooting");
	clearInterval(interval);
	videoSink.stop();
	videoTrack.stop();
	peerConnection.close();
	io.close();
	server.close();
}
//CONTROL
const commands = {

	"downscale": function() { scale -= 10 } ,
	"upscale": function() { scale -= 10 } ,

	"resetondisc": function(socket) {
		resetautoondisc	= !resetautoondisc;
		if (resetautoondisc) {
			socket.emit("logmessage","Will automatically reset on disconnect");
		} else {
			socket.emit("logmessage","No longer automatically resetting on disconnect");
		}
	},

	"reset": function(socket) {
		connection_used = true;
		socket.emit("logmessage","Resetting ICE / RTCoffer, please wait.");
		createOffer()
	},

	"reboot": function(socket) {
		if (ready_to_send_offer == true) {
			socket.emit("logmessage","Rebooting server.");
			reboot();
		} else {
			socket.emit("logmessage","Cannot reboot yet, offer generation still in progress");
		}
	},

	"nuke": function(socket) {
		socket.emit("logmessage","Rebooting server.");
		reboot();
    	throw new Error('forcing node server to stop through an error being thrown');
	},

	"lock": function(socket) {
		fulllock = false;
		socket.emit("logmessage","Locked access");
	},
	
	"refuse1337": function(socket) {
		if (refuseconnections == true) {
			refuseconnections = false
			socket.emit("logmessage","_");
		} else {
			refuseconnections = true
			socket.emit("logmessage","#");	
		}
	},

}

//NETWORKING
io.on('connection', (socket) => {
	console.log("a user connected");
	makeframes = true;

	if (ready_to_send_offer == false) {
		socket.emit("logmessage","ICE / RTCOffer has not been established yet, please wait.");
	} else {
		socket.emit("logmessage","ICE has been established!");
		socket.emit("RTCoffer",peerConnection.localDescription);
	}

	socket.on("RTCanswer", (answer) => {
		console.log(refuseconnections);
		if (refuseconnections == false) {
			console.log(answer);
			establishAnswer(answer);
		}
	})

	socket.on("bandwidth", (response) => {
		//console.log(response)
		//bandwidth = response;
	})

	socket.on("control", (response) => {
		switch (response.control) {
			case MouseEvents.MOVEMENT:		
				screengrabber.MoveMouse(response.deltax, response.deltay, response.immersivemode);
				break;
			case MouseEvents.KEYBOARD:
				screengrabber.KeyPress(response.key, response.up);
				break;
			default:
				//console.log(response.control);
				screengrabber.MouseEvent(-1,-1,response.control,response.param);
		}	
	})


	socket.on("quality",(response) => {
		scale = response;
	})

	socket.on("framerate",(response) => {
		startFrameCap(response)
	})

	socket.on("command", (response) => {
		if (commands[response] == null) {
			socket.emit("logmessage","Not recognised.");
		} else {
			commands[response](socket);
		}
	})

	socket.on("disconnect", () => {
		makeframes = false;
		refuseconnections = true
		console.log("user disconnected");

		if (resetautoondisc == true) {
			createOffer();
		}
	})
})

app.use(express.static("website/design"));

app.get('/lock_qwertyuiop', (req,res) => {
	fulllock = !fulllock // locks the website so that no one gets any HTTP response unless "lock_qwertyuiop" has been entered
})

app.get('/cobertverified', (req,res) => {
	console.log(req.connection.remoteAddress)
	res.sendFile(__dirname + "/website/index.html"); //serve HTML
})

server.listen(9999, () => {
	console.log("listening on 9999");
})
