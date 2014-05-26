var drawingApp = (function() {
   "use strict";
   
   var canvas,
   ctx,
   
   
   log = function(x) {
     console.log(x);
   },

   initCanvas = function() {
	 canvas = document.getElementById('myCanvas');
	 ctx = canvas.getContext('2d');
   },

   dot = function( x, y ){
	 ctx.fillRect( x, y, 1, 1);
   },

   drawSomething = function(){
	 var i = 0;
	 var x, y, len, x1, y1
  
	 i = 0;
	 while( i < 361 ) {
	   x = i;
	   y = Math.sin( x * (Math.PI / 180)  ) * 60;
	   dot( x/2, y + 100 );
	   i ++
	 }
	 log("draw sin");
  
	 x1 = 300;
	 y1 = 300;
	 len = 100
	 i = 0;
	 while( i < 900 ) {
	   x = x1 + len * Math.cos( i/2 * (Math.PI / 180)  );
	   y = y1 + len * Math.sin( i/2 * (Math.PI / 180)  );
	   dot( x, y );
	   i ++;
	 }
	 log("draw circle");
   },
   
   mainExec = function() {
	 initCanvas();
	 drawSomething();
   };
  
   return {
	 main: mainExec
   };

}());