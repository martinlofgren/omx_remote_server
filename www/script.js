var sock = new WebSocket("ws://localhost:12321/ws");
var stopButton = document.getElementById("stopButton");
var playPauseButton = document.getElementById("playPauseButton");
var titleDiv = document.getElementById("title");
var infoUrlDiv = document.getElementById("info-url");
var streamUrlDiv = document.getElementById("stream-url");
var durationDiv = document.getElementById("position");
var positionDiv = document.getElementById("duration");
var textInput = document.getElementById("textinput");

textInput.addEventListener("keyup", function(e) {
    if (e.keyCode === 13 && textInput.value) {
        sock.send("open " + textInput.value);
    }
}, false);

sock.addEventListener("message", function(e) {
    console.log(e.data);
    e.data.split('\n').forEach(function(element) {
        var a = element.split(':');
        if (a[0]) {
            switch(a[0].trim()) {
            case "status":
		switch(a[1].trim()) {
                case "playing":
                    playPauseButton.enabled=true;
                    playPauseButton.innerHTML="Pause";
                    break;
                case "paused":
                    playPauseButton.enabled=true;
                    playPauseButton.innerHTML="Play";
                    break;
                case "stopped":
                    playPauseButton.enabled=false;
                    break;
		}
		break;
            case "position":
		positionDiv.innerHTML = a[1].trim();
		break;
            default:
		var span = document.getElementById(a[0].trim());
		if (span) {
                    span.innerHTML = a[1].trim();
		}
		sock.send("status");
            }
        }
    });
}, false);
