$(document).ready(function() {

  peerServer = new Peer('whatthefuck', { key: '5xokte33u1ve7b9', debug: 3});

  console.log( "open server" );
  peerServer.on('open', function(id) {
    console.log( "server opened: " + id );
  });



  console.log( "call on connection 1" );

  peerServer.on('connection', function(connection) {
    console.log("connection from peer");

    connection.on('open', function(){
      console.log("connection opened");
      connection.send("serverfunck you");
    });

    connection.on('data', function(data){
      console.log("got data: " + data );
    });

  });

  console.log( "call on connection 2" );

});
