$(document).ready(function() {
    var ws = new WebSocket('ws://' + window.location.hostname + ':81/');
    ws.onmessage = function(event) { processReceivedCommand(event); };

    function processReceivedCommand(evt) {
        console.log("event")
        console.log(evt.data);
    }

    $(".slider").each(function(index) {
        console.log($(this).data())
        noUiSlider.create(this, {
            start: $(this).data("start"),
            range: {
                'min': $(this).data("min"),
                'max': $(this).data("max")
            },
        })
        let slider = this
        slider.noUiSlider.on('change', function(){
            let data = {
                'msgType': $(slider).data("msgtype"),
                'val': slider.noUiSlider.get(),
            };
            console.log(data)
            ws.send(JSON.stringify(data));
        });
    })

    $(".inputBtn").click(function(event) {
        console.log($(event.currentTarget))
        let data = {
            'msgType': $(event.currentTarget).data("msgtype")
        }
        console.log(data)
        ws.send(JSON.stringify(data));
    })


    // $("#.form-control").on("input", function(event) {
    //     let data = {
    //         'msgType': $(event.currentTarget).data("msgType"),
    //         'val': $(event.currentTarget).val(),
    //     };
    //     ws.send(JSON.stringify(data));
        
    // })

    // $("#randomBtn").click(function() {
    //     let data = {
    //         'msgType': 1,
    //     }
        
    // })

    // $("#rainbowBtn").click(function() {
    //     let data = {
    //         'msgType': 2,
    //     }
        
    // })

    // $("#tranquilityBtn").click(function() {
    //     let data = {
    //         'msgType': 3,
    //     }
        
    // })

    // $("#paletteBtn").click(function() {
    //     let data = {
    //         'msgType': 4,
    //     }
        
    // })

    // $("#tripperBtn").click(function() {
    //     let data = {
    //         'msgType': 5,
    //     }
        
    // })

    // $("#antsBtn").click(function() {
    //     let data = {
    //         'msgType': 6,
    //     }
        
    // })

    // $("#starsBtn").click(function() {
    //     let data = {
    //         'msgType': 7,
    //     }
        
    // })

    // $("#houseLightsBtn").click(function() {
    //     let data = {
    //         'msgType': 8,
    //     }
        
    // })

    // $("#launchBtn").click(function() {
    //     let data = {
    //         'msgType': 9,
    //     }
        
    // })

    // $("#noiseBtn").click(function() {
    //     let data = {
    //         'msgType': 14,
    //     }
        
    // })

    // $("#maskBtn").click(function() {
    //     let data = {
    //         'msgType': 15,
    //     }
        
    // })

    // $("#earthBtn").click(function() {
    //     let data = {
    //         'msgType': 20,
    //     }
        
    // })

    // $("#fireBtn").click(function() {
    //     let data = {
    //         'msgType': 21,
    //     }
        
    // })

    // $("#airBtn").click(function() {
    //     let data = {
    //         'msgType': 22,
    //     }
        
    // })

    // $("#waterBtn").click(function() {
    //     let data = {
    //         'msgType': 23,
    //     }
        
    // })

    // $("#metalBtn").click(function() {
    //     let data = {
    //         'msgType': 24,
    //     }
        
    // })

    // $("#stepInput").on("input", function() {
    //     let data = {
    //         'msgType': 10,
    //         'val': $("#stepInput").val(),
    //     }
        
    // })

    // $("#fadeInput").on("input", function() {
    //     let data = {
    //         'msgType': 11,
    //         'val': $("#fadeInput").val(),
    //     }
        
    // })

    // $("#brightnessInput").on("input", function() {
    //     let data = {
    //         'msgType': 12,
    //         'val': $("#brightnessInput").val(),
    //     }
        
    // })

    // $("#ripplesInput").on("input", function() {
    //     let data = {
    //         'msgType': 13,
    //         'val': $("#ripplesInput").val(),
    //     }
        
    // })

    // $("#nlInput").on("input", function() {
    //     let data = {
    //         'msgType': 16,
    //         'val': $("#nlInput").val(),
    //     }
        
    // })

    // $("#nsInput").on("input", function() {
    //     let data = {
    //         'msgType': 17,
    //         'val': $("#nsInput").val(),
    //     }
        
    // })

    // $("#nflInput").on("input", function() {
    //     let data = {
    //         'msgType': 18,
    //         'val': $("#nflInput").val(),
    //     }
        
    // })

    // $("#nfsInput").on("input", function() {
    //     let data = {
    //         'msgType': 19,
    //         'val': $("#nfsInput").val(),
    //     }
        
    // })

    // $("#airlInput").on("input", function() {
    //     let data = {
    //         'msgType': 25,
    //         'val': $("#airlInput").val(),
    //     }
        
    // })

    // $("#airsInput").on("input", function() {
    //     let data = {
    //         'msgType': 26,
    //         'val': $("#airsInput").val(),
    //     }
        
    // })

    // $("#ripplesBtn").click(function() {
    //     let data = {
    //         'msgType': 28,
    //     }
        
    // })

    // $("#tailBtn").click(function() {
    //     let data = {
    //         'msgType': 29,
    //     }
        
    // })

    // $("#blenderBtn").click(function() {
    //     let data = {
    //         'msgType': 30,
    //     }
        
    // })

    // $("#flashBtn").click(function() {
    //     let data = {
    //         'msgType': 31,
    //     }
        
    // })
})