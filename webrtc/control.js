var controlApp = (function(){

  var canvas,
  peerControl,
  connect, 
  
  
  connectInit = function() {
    peerControl = new Peer('whatthefuck_c', { key: '5xokte33u1ve7b9', debug: 3});
    connect = peerControl.connect('whatthefuck');

    connect.on('open', function() {
      console.log("connection open");
      connect.on('data', function(data) {
        console.log("from server", data);
      });
    });
  }, 
 
  mainExec = function() {
    connectInit();
    
    canvas = document.getElementById('myCanvas');
  
  	canvas.addEventListener('touchmove', function(event) {
      var rect = canvas.getBoundingClientRect();
    
      for (var i = 0; i < event.touches.length; i++) {
        var touch = event.touches[i];
        console.log(touch.pageX, touch.pageY);
        var pos = {};
        pos.x = touch.pageX - rect.left;
        pos.y = touch.pageY - rect.top;
        connect.send(pos);
      }
    });
  
  	canvas.addEventListener('touchstart', function(event) {
  	  console.log("touchstart");
  	});
  	
  	canvas.addEventListener('touchend', function() {
      console.log("touchend");
    });
  };

  return {
    main: mainExec
  };

}());
