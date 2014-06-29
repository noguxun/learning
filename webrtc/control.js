$(document).ready(function() {
  var canvas;
  var peerControl;
  var connect;

  peerControl = new Peer('whatthefuck_c', { key: '5xokte33u1ve7b9', debug: 3});
  connect = peerControl.connect('whatthefuck');

  connect.on('open', function() {
    console.log("connection open");
    connect.on('data', function(data) {
      console.log("from server", data);
    });
    connect.send("hello");
  });

  //canvas = $("#mycanvas");
  canvas = document.getElementById('mycanvas');
  canvas.addEventListener('touchend', function() {
    connect.send("touch end");
    console.log("touchend");
  });

  canvas.addEventListener('touchmove', function(event) {
    //connect.send(event.touches);
    for (var i = 0; i < event.touches.length; i++) {
      var touch = event.touches[i];
      console.log(touch.pageX, touch.pageY);
      var pos = {};
      pos.x = touch.pageX;
      pos.y = touch.pageY;
      connect.send(pos);
    }
  });

  canvas.addEventListener('touchstart', function(event) {
    //connect.send(event);
    console.log(event);
  });

});
