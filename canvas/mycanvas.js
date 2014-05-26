var drawingApp = (function() {
   "use strict";
   
   var canvas,
   ctx,
   intervalHandle,
   angle = 0,
   
   
   log = function(x) {
     console.log(x);
   },

   initCanvas = function() {
	 canvas = document.getElementById('myCanvas');
	 ctx = canvas.getContext('2d');
	 intervalHandle = setInterval(draw, 10)
   },

   dot = function( x, y ){
	 ctx.fillRect( x, y, 1, 1);
   },

   draw = function(){
	 var x, y, len, x1, y1, i;
  
	 x = angle;
	 y = Math.sin( x * (Math.PI / 180)  ) * 60;
	 dot( x/2, y + 100 );
	 angle ++
	 
	 log("draw sin");
  
	 x1 = 300;
	 y1 = 300;
	 len = 100
	 
	 x = x1 + len * Math.cos( angle/2 * (Math.PI / 180)  );
	 y = y1 + len * Math.sin( angle/2 * (Math.PI / 180)  );
     dot( x, y );
	   
	 log("draw circle");
   },
   
   mainExec = function() {
	 initCanvas();
	 draw();
   };
  
   return {
	 main: mainExec
   };

}());