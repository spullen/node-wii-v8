var EventEmitter = require("events").EventEmitter;
var WiiMote = require("./build/Release/wiiv8.node");

var wiimote = new WiiMote();

for(var key in EventEmitter.prototype) {
    wiimote[key] = EventEmitter.prototype[key];
}

wiimote.setEmitter(function(event,data){
    wiimote.emit(event,data);
});

module.exports.WiiMote = wiimote;