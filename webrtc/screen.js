var screenApp = (function(){
  "use strict";
  
  var canvas,
  ctx,
  peerServer, 
  
  clearRect = function(){
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    // Store the current transformation matrix
    ctx.save();

    // Use the identity matrix while clearing the canvas
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Restore the transform
    ctx.restore();
  },
  
  
  drawFinder = function(cx, cy, radius, color){
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, 2*Math.PI, false);
    ctx.moveTo(cx - radius - 4, cy);
    ctx.lineTo(cx + radius + 4, cy);
    ctx.moveTo(cx, cy - radius - 4);
    ctx.lineTo(cx, cy + radius + 4);
    ctx.strokeStyle = color;
    ctx.stroke();
  },
  
  mainExec = function() {
    canvas = document.getElementById('myCanvas');
    ctx = canvas.getContext('2d');
    
    peerServer = new Peer('whatthefuck', { key: '5xokte33u1ve7b9', debug: 3});
    peerServer.on('open', function(id) {
    	console.log( "server opened: " + id );
  	});
  	
  	peerServer.on('connection', function(connection) {
      console.log("connection from peer");

      connection.on('open', function(){
        console.log("connection opened");
        connection.send("serverfunck you");
      });

      connection.on('data', function(data){
        console.log( data );      
        clearRect();
        drawFinder( data.x, data.y, 10, "#0f0f0f" );
      });
    });
  };

  return {
    main: mainExec
  };

}());
