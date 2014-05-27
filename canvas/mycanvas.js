var drawingApp = (function(){
   "use strict";

   var canvas,
   ctx,
   intervalHandle,
   angle1 = 0,
   angle2 = 0,
   angle3 = 0,


   log = function(x){
     console.log(x);
   },

   initCanvas = function(){
     canvas = document.getElementById('myCanvas');
     ctx = canvas.getContext('2d');
     intervalHandle = setInterval(draw2, 100);
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

   circle = function(cx, cy, radius, angle){

     var x, y;
     x = cx + radius * Math.cos(angle * (Math.PI / 180));
     y = cy + radius * Math.sin(angle * (Math.PI / 180));

     ctx.beginPath();
     ctx.arc(cx, cy, radius, 0, 2*Math.PI, false);
     ctx.moveTo(cx, cy);
     ctx.lineTo(x, y);
     ctx.stroke();

     return { x: x, y: y};
   },

   updateAngle = function() {
     angle1 += 1;
     if(angle1 == 360){
       angle1 = 0;
     }
     angle2 += 2;
     if(angle2 == 360){
       angle2 = 0;
     }
     angle3 += 5;
     if(angle3 == 360){
       angle3 = 0;
     }
   },

   draw2 = function(){
     var dot1 = {};
     var dot2 = {};

     clearRect();
     dot1 = circle(300, 300, 100, angle1);
     dot2 = circle(dot1.x, dot1.y, 50, angle2);
     circle(dot2.x, dot2.y, 25, angle3);

     updateAngle();
  },

   mainExec = function() {
     initCanvas();
     draw2();
   };

   return {
     main: mainExec
   };

}());
