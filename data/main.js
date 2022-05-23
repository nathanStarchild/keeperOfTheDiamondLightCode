$(document).ready(function() {
    var ws = new WebSocket('ws://' + window.location.hostname + ':81/');
    ws.onmessage = function(event) { processReceivedCommand(event); };

    function processReceivedCommand(evt) {
        console.log(evt.data);
    }

    $("#randomBtn").click(function() {
        let data = {
            'msgType': 1,
        }
        ws.send(JSON.stringify(data));
    })

    $("#rainbowBtn").click(function() {
        let data = {
            'msgType': 2,
        }
        ws.send(JSON.stringify(data));
    })

    $("#tranquilityBtn").click(function() {
        let data = {
            'msgType': 3,
        }
        ws.send(JSON.stringify(data));
    })

    $("#paletteBtn").click(function() {
        let data = {
            'msgType': 4,
        }
        ws.send(JSON.stringify(data));
    })

    $("#tripperBtn").click(function() {
        let data = {
            'msgType': 5,
        }
        ws.send(JSON.stringify(data));
    })

    $("#antsBtn").click(function() {
        let data = {
            'msgType': 6,
        }
        ws.send(JSON.stringify(data));
    })

    $("#starsBtn").click(function() {
        let data = {
            'msgType': 7,
        }
        ws.send(JSON.stringify(data));
    })

    $("#houseLightsBtn").click(function() {
        let data = {
            'msgType': 8,
        }
        ws.send(JSON.stringify(data));
    })

    $("#launchBtn").click(function() {
        let data = {
            'msgType': 9,
        }
        ws.send(JSON.stringify(data));
    })

    $("#noiseBtn").click(function() {
        let data = {
            'msgType': 14,
        }
        ws.send(JSON.stringify(data));
    })

    $("#maskBtn").click(function() {
        let data = {
            'msgType': 15,
        }
        ws.send(JSON.stringify(data));
    })

    $("#earthBtn").click(function() {
        let data = {
            'msgType': 20,
        }
        ws.send(JSON.stringify(data));
    })

    $("#fireBtn").click(function() {
        let data = {
            'msgType': 21,
        }
        ws.send(JSON.stringify(data));
    })

    $("#airBtn").click(function() {
        let data = {
            'msgType': 22,
        }
        ws.send(JSON.stringify(data));
    })

    $("#waterBtn").click(function() {
        let data = {
            'msgType': 23,
        }
        ws.send(JSON.stringify(data));
    })

    $("#metalBtn").click(function() {
        let data = {
            'msgType': 24,
        }
        ws.send(JSON.stringify(data));
    })

    $("#stepInput").on("input", function() {
        let data = {
            'msgType': 10,
            'val': $("#stepInput").val(),
        }
        ws.send(JSON.stringify(data));
    })

    $("#fadeInput").on("input", function() {
        let data = {
            'msgType': 11,
            'val': $("#fadeInput").val(),
        }
        ws.send(JSON.stringify(data));
    })

    $("#brightnessInput").on("input", function() {
        let data = {
            'msgType': 12,
            'val': $("#brightnessInput").val(),
        }
        ws.send(JSON.stringify(data));
    })

    $("#ripplesInput").on("input", function() {
        let data = {
            'msgType': 13,
            'val': $("#ripplesInput").val(),
        }
        ws.send(JSON.stringify(data));
    })

    $("#nlInput").on("input", function() {
        let data = {
            'msgType': 16,
            'val': $("#nlInput").val(),
        }
        ws.send(JSON.stringify(data));
    })

    $("#nsInput").on("input", function() {
        let data = {
            'msgType': 17,
            'val': $("#nsInput").val(),
        }
        ws.send(JSON.stringify(data));
    })

    $("#nflInput").on("input", function() {
        let data = {
            'msgType': 18,
            'val': $("#nflInput").val(),
        }
        ws.send(JSON.stringify(data));
    })

    $("#nfsInput").on("input", function() {
        let data = {
            'msgType': 19,
            'val': $("#nfsInput").val(),
        }
        ws.send(JSON.stringify(data));
    })

    $("#airlInput").on("input", function() {
        let data = {
            'msgType': 25,
            'val': $("#airlInput").val(),
        }
        ws.send(JSON.stringify(data));
    })

    $("#airsInput").on("input", function() {
        let data = {
            'msgType': 26,
            'val': $("#airsInput").val(),
        }
        ws.send(JSON.stringify(data));
    })
})