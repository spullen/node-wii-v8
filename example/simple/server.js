var wii = require("../../wii");
var express = require("express");
var app = express();
var http = require("http");
var server = http.createServer(app);
var querystring = require('querystring');
var http = require('http');
var request = require('request');

server.listen(8888);


var SegfaultHandler = require('segfault-handler');
SegfaultHandler.registerHandler("crash.log");

var hue = require("node-hue-api"),
    HueApi = hue.HueApi,
    hostname = "192.168.86.104",
    username = "2f95843c151d9171a04ebef12a53f4b",
    api = new HueApi(hostname, username),
    wiimote = wii.WiiMote;

var maddiesBedroom = 0,
    masterBedroom = 0,
    livingRoom = 0,
    entrance = 0,
    partyMode;

console.log("Put wiimote in discoverable mode...");

wiimote.connect("00:00:00:00:00:00", function (err) {
    if (err) {
        console.log("Could not establish connection");
        return;
    }
    console.log("wiimote connected");

    wiimote.led(1, true);

    wiimote.rumble(true);
    setTimeout(function () {
        wiimote.rumble(false);
    }, 100);

    wiimote.on("button", function (data) {

        var BTN_1 = 2,
            BTN_2 = 1,
            BTN_A = 8,
            BTN_B = 4,
            BTN_MINUS = 16,
            BTN_PLUS = 4096,
            BTN_HOME = 128,
            BTN_LEFT = 256,
            BTN_RIGHT = 512,
            BTN_UP = 2048,
            BTN_DOWN = 1024;


        // var bedroom = [2, 3, 5],
        //     livingRoom = [6, 7],
        //     entrance = 1,
        //     tvLightStrip = 4,
        //     maddieRoom = [8, 9, 10];

        var state = hue.lightState.create().turnOn();

        switch (data) {
            case BTN_A:
                switch (maddiesBedroom){
                    case 0: // Nightlight
                        console.log("Maddie's Room Nightlight");
                        setScene(5, 'Hq9JIKABl-dDOvp');
                        maddiesBedroom++;
                        break;
                    case 1: // Dimmed
                        console.log("Maddie's Room Dimmed");
                        setScene(5, '9DM2iYXDpSrU1SQ');
                        maddiesBedroom++;
                        break;
                    case 2: // Bright
                        console.log("Maddie's Room Bright");
                        setScene(5, 'y-eGZy7UqliB-Vp');
                        maddiesBedroom++;
                        break;
                    default: // Off
                        console.log("Maddie's Room Off");
                        setScene(5, null);
                        maddiesBedroom = 0;
                        break;

                }
                break;

            case BTN_HOME:
                switch (masterBedroom){
                    case 0: // Nightlight
                        console.log("Master Bedroom Nightlight");
                        setScene(2, '6NiSFrmNpo5h3rp');
                        masterBedroom++;
                        break;
                    case 1: // Dimmed
                        console.log("Master Bedroom Dimmed");
                        setScene(2, 'P-y2R2T8yF2GMok');
                        masterBedroom++;
                        break;
                    case 2: // Bright
                        console.log("Master Bedroom Bright");
                        setScene(2, 'f3qR8ZgVjQP0WX0');
                        masterBedroom++;
                        break;
                    default: // Off
                        console.log("Master Bedroom Off");
                        setScene(2, null);
                        masterBedroom = 0;
                        break;

                }
                break;

            case BTN_MINUS:
                console.log("Master Concentrate");
                setScene(2, 'KvcWeXzsRlI8IS6');
                masterBedroom = 0;
                break;

            case BTN_PLUS:
                console.log("Master Arctic Aurora");
                setScene(2, '7i5HtteI0DKhA5S');
                masterBedroom = 0;
                break;

            case BTN_B:
                console.log("All Off");
                setLightState(1, false);
                setLightState(2, false);
                setLightState(3, false);
                setLightState(4, false);
                setLightState(5, false);
                setLightState(6, false);
                setLightState(7, false);
                setLightState(8, false);
                setLightState(9, false);
                setLightState(10, false);
                livingRoom = 0;
                masterBedroom = 0;
                maddiesBedroom = 0;
                entrance = 0;
                break;


            case BTN_1:
                switch (livingRoom){
                    case 0: // Nightlight
                        console.log("Living Room Nightlight");
                        setScene(1, 'YxcZBpe4r3fh8AE');
                        livingRoom++;
                        break;
                    case 1: // Dimmed
                        console.log("Living Room Dimmed");
                        setScene(1, 'ekJdinfETeCrYKB');
                        livingRoom++;
                        break;
                    case 2: // Bright
                        console.log("Living Room Bright");
                        setScene(1, '2TqvhneCzkRPF5g');
                        livingRoom++;
                        break;
                    default: // Off
                        console.log("Living Room Off");
                        setScene(1, null);
                        livingRoom = 0;
                        break;

                }
                break;

            case BTN_2:
                switch (entrance){
                    case 0: // Nightlight
                        console.log("Entrance Nightlight");
                        setScene(4, '9deXIqp-WE2JmPm');
                        entrance++;
                        break;
                    case 1: // Dimmed
                        console.log("Entrance Dimmed");
                        setScene(4, 'JcB1u5xzxAJGnFj');
                        entrance++;
                        break;
                    case 2: // Bright
                        console.log("Entrance Bright");
                        setScene(4, 'BsbVglErxYIBP4P');
                        entrance++;
                        break;
                    default: // Off
                        console.log("Living Room Off");
                        setScene(4, null);
                        entrance = 0;
                        break;

                }
                break;

            case BTN_UP:
                console.log("Living Room Party");

                if(partyMode){
                    clearTimeout(partyMode);
                    setLightState(1,false);
                    setLightState(6,false);
                    setLightState(7,false);
                    setLightState(4,false);
                } else {
                    partyMode = setTimeout(function(){
                        setLightState(1,true);
                        setLightState(6,true);
                        setLightState(7,true);
                        setLightState(4,true);
                    },100);

                }
                break;
        }
    });

    wiimote.on("connect", function (data) {
        io.sockets.emit("connect", data);
    });

    wiimote.on("disconnect", function (data) {
        io.sockets.emit("disconnect", data);
    });

    wiimote.button(true);

});

function setLightState(id, state){
    var options = {
        uri: 'http://' + hostname + '/api/' + username + '/lights/' + id + '/state',
        method: 'PUT',
        json: {
            on: state,
            bri: Math.floor(Math.random() * 254),
            hue: Math.floor(Math.random() * 65535),
            transitiontime: 3
        }
    };

    request(options, function (error, response, body) {
    });
}

// /api/<username>/groups/<id>/action
function setScene(groupId, scene){

    var json = {};

    if(scene){
        json.scene = scene;
        json.transitiontime = 0;
    } else {
        json.on = false;
    }

    var options = {
        uri: 'http://' + hostname + '/api/' + username + '/groups/' + groupId + '/action',
        method: 'PUT',
        json: json
    };

    request(options, function (error, response, body) {
        if (!error && response.statusCode == 200) {
            // console.log(body.id); // Print the shortened url.
        }
    });
}