var drawingApp = (function(){
   "use strict";

   var canvas,
   ctx,
   intervalHandle,
   cirMods = [
       { radius: 100, step: 1, color: "#ff0000", angle: 0 },
       { radius: 50, step: 2, color: "purple", angle: 0 },
       { radius: 20, step: 4, color: "blue", angle: 0 },
     ],
   centerX = 300,
   centerY = 300,

   log = function(x){
     console.log(x);
   },

   initCanvas = function(){
     canvas = document.getElementById('myCanvas');
     ctx = canvas.getContext('2d');
     intervalHandle = setInterval(draw2, 50);
   },

   dot = function( x, y ){
     ctx.fillRect( x, y, 1, 1);
   },

   clearRect = function() {
     ctx.clearRect(0, 0, canvas.width, canvas.height);
     // Store the current transformation matrix
     ctx.save();

     // Use the identity matrix while clearing the canvas
     ctx.setTransform(1, 0, 0, 1, 0, 0);
     ctx.clearRect(0, 0, canvas.width, canvas.height);

     // Restore the transform
     ctx.restore();
   },

   draw1 = function(){
     var x, y, i;
     x = angle;
     y = Math.sin( x * (Math.PI / 180)  ) * 60;
     dot( x, y + 100 );
     angle ++

     log("draw sin");
   },

   circle = function(cx, cy, radius, angle, color){

     var x, y;
     x = cx + radius * Math.cos(angle * (Math.PI / 180));
     y = cy + radius * Math.sin(angle * (Math.PI / 180));

     ctx.beginPath();
     ctx.arc(cx, cy, radius, 0, 2*Math.PI, false);
     ctx.moveTo(cx, cy);
     ctx.lineTo(x, y);
     ctx.strokeStyle = color;
     ctx.stroke();

     return { x: x, y: y};
   },

   updateModel = function() {
     for(var i = 0; i< cirMods.length; i++){
       cirMods[i].angle += cirMods[i].step;
       if(cirMods[i].angle == 360){
         cirMods[i].angle = 0;
       }
     }
   },

   draw2 = function(){
     var dot = {};

     clearRect();
     dot = circle(centerX, centerY, cirMods[0].radius, cirMods[0].angle, cirMods[0].color);
     for(var i = 1; i< cirMods.length; i++){
       dot = circle(dot.x, dot.y, cirMods[i].radius, cirMods[i].angle, cirMods[i].color);
     }

     updateModel();
  },

   mainExec = function() {
     initCanvas();
     draw2();
   };

   return {
     main: mainExec
   };

}());
