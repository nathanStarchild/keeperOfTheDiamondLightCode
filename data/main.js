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
            direction: $(this).data("direction"),
            step: 1,
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

    $(".caret").click(function(){
        console.log("clicked")
        $(this).toggleClass("rotated")
    });

    $('#paletteSelect').on('change', function(){
        let data = {
            'msgType': $(this).data("msgtype"),
            'val': $(this).val(),
        };
        console.log(data)
        ws.send(JSON.stringify(data));
        
    })

    $("#customPaletteBtn").click(function(event) {
        console.log($(event.currentTarget))
        let pal = getOutputPalette().flat();
        let data = {
            'msgType': $(event.currentTarget).data("msgtype"),
            'palette': pal
        }
        console.log(data)
        ws.send(JSON.stringify(data));
    })

    /////////////////////////////////////////////////////////////
    // Custom Palette Biz

    let designPalette = [
        [211, 54, 130],
        [42, 161, 152]
    ]

    let activeStop = 0;

    function asRGBstring(c){
        return `rgba(${c[0]}, ${c[1]}, ${c[2]}, 1)`
    }

    var colourWheel = new iro.ColorPicker("#colorWheelDemo", {
        width: 180,
        display: "inline-block",
        layout: [
            {
                component: iro.ui.Wheel,
                options: {
                    wheelLightness: true,
                    wheelAngle: 0,
                    wheelDirection: "anticlockwise"
                } 
            },
            {
                component: iro.ui.Slider,
                options: {
                    sliderType: 'value'
                }
            }
        ]
    });

    function updateColourBar(pType){
        let cols = designPalette;
        cols.entries()
        let d = cols.length
        if (pType == "gradient"){
            d -= 1;
        }
        let frac = 100 / d;
        let tmpl = `linear-gradient(to right`
        for (const [i, c] of cols.entries()){

            if (pType == "gradient"){
                tmpl += `, ${asRGBstring(c)} ${frac*i}%`
            } else {
                if (i > 0){
                    tmpl += ` ${frac*i}%`
                }
                tmpl += `, ${asRGBstring(c)} ${frac*i}%, ${asRGBstring(c)}`
            }
        }
        tmpl += `)`
        console.log(tmpl)
        $("#colourBar").css("background-image", tmpl);
    }

    function updateColourStops(pType){
        console.log("something")
        $("#colourStops").html("")
        for (const [i, c] of designPalette.entries()) {
            let tmpl = `
                <div class="colourStopContainer d-flex flex-column justify-content-center">
                    <div class="pt-1 text-center">^</div>
                    <div id="colourStop-${i}" data-n="${i}" class="colourStop pointer" style="background-color: ${asRGBstring(c)};"}></div>
            `
            if (designPalette.length > 2){
                tmpl += `
                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-trash pointer mt-3 deleteStop" data-n="${i}" viewBox="0 0 16 16">
                        <path d="M5.5 5.5A.5.5 0 0 1 6 6v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5zm2.5 0a.5.5 0 0 1 .5.5v6a.5.5 0 0 1-1 0V6a.5.5 0 0 1 .5-.5zm3 .5a.5.5 0 0 0-1 0v6a.5.5 0 0 0 1 0V6z"></path>
                        <path fill-rule="evenodd" d="M14.5 3a1 1 0 0 1-1 1H13v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V4h-.5a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1H6a1 1 0 0 1 1-1h2a1 1 0 0 1 1 1h3.5a1 1 0 0 1 1 1v1zM4.118 4 4 4.059V13a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1V4.059L11.882 4H4.118zM2.5 3V2h11v1h-11z"></path>
                    </svg>
                `
            }
            tmpl += `
                </div>
            `
            if (i < designPalette.length - 1 && designPalette.length < 16){
                tmpl += `
                    <div class="colourStopContainer d-flex flex-column justify-content-center fs-3 pointer addStop" data-n="${i+1}">
                        <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" fill="currentColor" class="bi bi-plus-square pointer" viewBox="0 0 16 16">
                            <path d="M14 1a1 1 0 0 1 1 1v12a1 1 0 0 1-1 1H2a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1h12zM2 0a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V2a2 2 0 0 0-2-2H2z"></path>
                            <path d="M8 4a.5.5 0 0 1 .5.5v3h3a.5.5 0 0 1 0 1h-3v3a.5.5 0 0 1-1 0v-3h-3a.5.5 0 0 1 0-1h3v-3A.5.5 0 0 1 8 4z"></path>
                        </svg>
                    </div>
                `
            }
            $("#colourStops").append(tmpl)
        }
        if (pType == "solid") {
            $("#colourStops").removeClass("justify-content-between")
            $("#colourStops").addClass("justify-content-around")
        } else {
            $("#colourStops").addClass("justify-content-between")
            $("#colourStops").removeClass("justify-content-around")
        }
    }

    function updateActiveStop(){
        $(".activeStop").removeClass("activeStop")
        $(`#colourStop-${activeStop}`).addClass("activeStop")
        colourWheel.setColors([asRGBstring(designPalette[activeStop])], 0);
    }

    function updatePalette() {
        let pType = $("input[name=platteType]:checked").val();
        updateColourBar(pType);
        updateColourStops(pType);
        updateActiveStop();
    }
    updatePalette()
    
    $("input[name=platteType]").change(updatePalette)

    $(document).on("click", ".colourStop", function(){
        activeStop = Number($(this).data("n"))
        updateActiveStop();
    })

    $(document).on("click", ".addStop", function(){
        if (designPalette.length == 16) {
            return
        }
        idx = Number($(this).data("n"))
        designPalette.splice(idx, 0, [255, 0, 255])
        activeStop = idx
        updatePalette();
    })

    $(document).on("click", ".deleteStop", function(){
        if (designPalette.length == 2) {
            return
        }
        idx = Number($(this).data("n"))
        designPalette.splice(idx, 1)
        activeStop = Math.min(activeStop, designPalette.length - 1)
        updatePalette();
    })

    colourWheel.on('color:change', function(colour, changes){
        // console.log(colour.rgb);
        designPalette[activeStop] = [colour.rgb.r, colour.rgb.g, colour.rgb.b]
        updatePalette();
        
    })

    function getOutputPalette() {
        let nOut = 16;
        let cols = designPalette
        let n = cols.length;
        let pType = $("input[name=platteType]:checked").val();
        let pOut
        if (pType == "solid"){
            let repeats = Math.floor(nOut/n);
            let nExtra = nOut % n;
            pOut = cols.map((elm, i) => {
                let r = i < nExtra ? repeats + 1 : repeats
                return Array(r).fill(elm)
            }).flat()
            console.log(pOut.length)
        } else {
            let step = (n-1) / (nOut-1)
            console.log(step)
            pOut = Array(nOut-2).fill().map((_, i) => {
                let v = step * (i + 1)
                // console.log(v)
                let s = Math.floor(v)
                let f = v - s
                // console.log(i)
                // console.log(s)
                // console.log(f)
                // console.log(cols[s])
                // console.log(cols[s+1])
                // console.log()
                console.log(`${s}, ${f}`)
                return lerpColour(cols[s], cols[s+1], f)
                // return [v, s, f]
            })
            console.log(pOut)
            pOut.unshift(cols[0])
            pOut.push(cols[cols.length-1])
        }
        // designPalette = pOut
        // updatePalette()
        return pOut

    }

    function lerp(a, b, n) {
     return (1 - n) * a + n * b;
    }

    function lerpColour(c1, c2, n) {
        return [
            Math.round(lerp(c1[0], c2[0], n)),
            Math.round(lerp(c1[1], c2[1], n)),
            Math.round(lerp(c1[2], c2[2], n)),
        ]
    }

    //////////////////////////////////////////////////



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