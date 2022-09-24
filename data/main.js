$(document).ready(function() {
    Object.defineProperty(String.prototype, 'capitalize', {
        value: function() {
          return this.charAt(0).toUpperCase() + this.slice(1);
        },
        enumerable: false
      });

    var ws = new WebSocket('ws://' + window.location.hostname + ':81/');
    ws.onmessage = function(event) { processReceivedCommand(event); };

    function processReceivedCommand(evt) {
        console.log("event")
        console.log(evt.data);
    }

    loadPatterns()

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

    $(".patternSlider").each(function(index) {
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
                'pointer': $(slider).data("pointer"),
                'param': $(slider).data("param"),
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

    $(".soloBtn").click(function(event) {
        console.log($(event.currentTarget))
        let data = {
            'msgType': $(event.currentTarget).data("msgtype"),
            'pointer': $(event.currentTarget).data("pattern")
        }
        console.log(data)
        ws.send(JSON.stringify(data));
    })


    $(".patternSwitch").on('change', function(event) {
        console.log($(event.currentTarget))
        let data = {
            'msgType': 38,
            'pointer': $(event.currentTarget).data("pattern"),
            'enabled': $(event.currentTarget).prop("checked")
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

    //Pattern biz

    function loadPatterns() {
        let patterns = [
        {
            "name": "tail",
            "pointer": 1,
            "enabled": false,
            "speed": {
                "direction": "ltr",
                "name": "speed",
                "min": -3,
                "max": 3,
                "start": 1,
            },
            "length": {
                "direction": "ltr",
                "name": "spacing",
                "min": 2,
                "max": 120,
                "start": 20
            },
            "decay": ""
        },
        {
            "name": "breathe",
            "pointer": 2,
            "enabled": false,
            "speed": {
                "direction": "ltr",
                "name": "bpm",
                "min": 6,
                "max": 60,
                "start": 12,
            },
            "length": "",
            "decay": ""
        },
        {
            "name": "glitter",
            "pointer": 3,
            "enabled": false,
            "speed": {
                "direction": "ltr",
                "name": "frequency",
                "min": 20,
                "max": 255,
                "start": 60,
            },
            "length": {
                "direction": "ltr",
                "name": "density",
                "min": 1,
                "max": 50,
                "start": 8,
            },
            "decay": {
                "direction": "ltr",
                "name": "colour",
                "min": 0,
                "max": 255,
                "start": 36,
            }
        },
        // {
        //     "name": "crazytown",
        //     "pointer": 4,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        // {
        //     "name": "enlightenment",
        //     "pointer": 5,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        {
            "name": "ripple",
            "pointer": 6,
            "enabled": false,
            "speed": "",
            "length": "",
            "decay": ""
        },
        {
            "name": "blendwave",
            "pointer": 7,
            "enabled": false,
            "speed": "",
            "length": "",
            "decay": "",
        },
        {
            "name": "rain",
            "pointer": 8,
            "enabled": false,
            "speed": {
                "direction": "rtl",
                "name": "min speed",
                "min": 5,
                "max": 50,
                "start": 15,
            },
            "length": {
                "direction": "ltr",
                "name": "drops",
                "min": 3,
                "max": 30,
                "start": 15
            },
            "decay": {
                "direction": "ltr",
                "name": "brightness",
                "min": 10,
                "max": 255,
                "start": 150
            }
        },
        // {
        //     "name": "holdingPattern",
        //     "pointer": 9,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        // {
        //     "name": "mapPattern",
        //     "pointer": 10,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        // {
        //     "name": "paletteDisplay",
        //     "pointer": 11,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        // {
        //     "name": "sweep",
        //     "pointer": 12,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        // {
        //     "name": "dimmer",
        //     "pointer": 13,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        // {
        //     "name": "skaters",
        //     "pointer": 14,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        // {
        //     "name": "poleChaser",
        //     "pointer": 15,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        // {
        //     "name": "powerSaver",
        //     "pointer": 16,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        {
            "name": "ants",
            "pointer": 17,
            "enabled": false,
            "speed": {
                "direction": "ltr",
                "name": "spacing",
                "min": 2,
                "max": 60,
                "start": 5,
            },
            "length": {
                "direction": "ltr",
                "name": "number",
                "min": 1,
                "max": 90,
                "start": 20
            },
            "decay": {
                "direction": "ltr",
                "name": "colour",
                "min": 0,
                "max": 255,
                "start": 55
            }
        },
        // {
        //     "name": "launch",
        //     "pointer": 18,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        // {
        //     "name": "houseLights",
        //     "pointer": 19,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        // },
        {
            "name": "spiral",
            "pointer": 20,
            "enabled": false,
            "speed": "",
            "length": "",
            "decay": ""
        },
        {
            "name": "rainbow",
            "pointer": 21,
            "enabled": false,
            "speed": {
                "direction": "ltr",
                "name": "speed",
                "min": 1,
                "max": 20,
                "start": 5,
            },
            "length": {
                "direction": "ltr",
                "name": "length",
                "min": 1,
                "max": 20,
                "start": 5
            },
            "decay": {
                "direction": "ltr",
                "name": "brightness",
                "min": 1,
                "max": 255,
                "start": 100
            }
        },
        {
            "name": "noise",
            "pointer": 22,
            "enabled": false,
            "speed": {
                "direction": "ltr",
                "name": "speed",
                "min": 1,
                "max": 30,
                "start": 2,
            },
            "length": {
                "direction": "rtl",
                "name": "length",
                "min": 1,
                "max": 50,
                "start": 5
            },
            "decay": ""
        },
        {
            "name": "shadow",
            "pointer": 23,
            "enabled": false,
            "speed": {
                "direction": "ltr",
                "name": "speed",
                "min": 1,
                "max": 30,
                "start": 2,
            },
            "length": {
                "direction": "rtl",
                "name": "length",
                "min": 1,
                "max": 50,
                "start": 5
            },
            "decay": ""
        },
        {
            "name": "air",
            "pointer": 24,
            "enabled": false,
            "speed": {
                "direction": "ltr",
                "name": "speed",
                "min": 2,
                "max": 7,
                "start": 3,
            },
            "length": {
                "direction": "ltr",
                "name": "length",
                "min": 1,
                "max": 10,
                "start": 3
            },
            "decay": ""
        },
        {
            "name": "fire",
            "pointer": 25,
            "enabled": false,
            "speed": {
                "direction": "ltr",
                "name": "sparking",
                "min": 3,
                "max": 60,
                "start": 30,
            },
            "length": "",
            "decay": {
                "direction": "ltr",
                "name": "cooling",
                "min": 10,
                "max": 80,
                "start": 30
            }
        },
        {
            "name": "the Void",
            "pointer": 27,
            "enabled": false,
            "speed": {
                "direction": "ltr",
                "name": "speed",
                "min": 1,
                "max": 10,
                "start": 1,
            },
            "length": {
                "direction": "ltr",
                "name": "length",
                "min": 5,
                "max": 255,
                "start": 30
            },
            "decay": {
                "direction": "ltr",
                "name": "fringe",
                "min": 5,
                "max": 255,
                "start": 30
            }
        }
        // {
        //     "name": "metal",
        //     "pointer": 26,
        //     "enabled": false,
        //     "speed": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": ,
        //     },
        //     "length": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start": 
        //     },
        //     "decay": {
        //         "direction": "ltr",
        //         "name": "",
        //         "min": ,
        //         "max": ,
        //         "start":
        //     }
        ]

        for (const pattern of patterns) {
            let tmpl = `
                <h4 class="gridLeft mt-2">${pattern.name.capitalize()}</h4>
                <div class="gridMiddle mt-2 d-flex flex-row justify-content-evenly w-100">
                    <div class="form-check form-switch">
                        <input class="form-check-input patternSwitch" type="checkbox" data-pattern="${pattern.pointer}">
                    </div>
                    <div data-msgtype="37" data-pattern="${pattern.pointer}" class="btn btn-secondary btn-sm soloBtn">Solo</div>
                </div>
                <button class="collapseBtn gridRight mt-2" type="button" data-bs-toggle="collapse" data-bs-target="#pattern${pattern.pointer}" ><img class="caret" src="caret-down.svg"></button>
                <div id="pattern${pattern.pointer}" class="collapse collapsed slider-container"> 
                    <div class="d-flex flex-column justify-content-center align-items-center gap-4">
            `
            for (const [i, param] of ["speed", "length", "decay"].entries()){
                if (pattern[param]){
                    tmpl += `
                        <label class="control slider-label">${pattern[param].name.capitalize()}:
                            <div data-msgtype="36" class="patternSlider" data-start="${pattern[param].start}" data-min="${pattern[param].min}" data-max="${pattern[param].max}" data-pointer="${pattern.pointer}" data-param="${i}"></div>              
                        </label> 
                    `
                }
            }
            if (pattern.name == "ripple") {
                tmpl += `
                    <label class="control slider-label">Number:
                        <div data-msgtype="13" class="slider" data-start="5" data-min="0" data-max="10"></div>              
                    </label> 
                `
            }
            tmpl += `</div></div>`
            $("#patternContainer").append(tmpl)
        }
    }

})