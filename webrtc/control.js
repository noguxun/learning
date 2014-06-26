$(document).ready(function() {

  peerControl = new Peer('whatthefuck_c', { key: '5xokte33u1ve7b9', debug: 3});

  var connect = peerControl.connect('whatthefuck');

  connect.on('open', function() {
    console.log("connection open");
    connect.on('data', function(data) {
      console.log("from server", data);
    });
    connect.send("hello");
  });

 });
