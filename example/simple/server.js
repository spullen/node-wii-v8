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

var wiimote = wii.WiiMote;

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

        switch (data) {
            case BTN_A:
                console.log('A Button Pressed');
                break;

            case BTN_HOME:
                console.log('Home Button Pressed');
                break;

            case BTN_MINUS:
                console.log('Minus Button Pressed');
                break;

            case BTN_PLUS:
                console.log('Plus Button Pressed');
                break;

            case BTN_B:
                console.log('B Button Pressed');
                break;


            case BTN_1:
                console.log('Button 1 Pressed');
                break;

            case BTN_2:
                console.log('Button 2 Pressed');
                break;

            case BTN_UP:
                console.log('Up Button Pressed');
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
