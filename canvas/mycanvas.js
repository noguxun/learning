var drawingApp = (function(){
  "use strict";

  var canvas,
  textbox,
  cirNum = 5,
  ctx,
  intervalHandle,
  cirMods = [
       { radius: (4 / Math.PI)*100,       step:  1, color:  "#ff0000",   angle: 0 },
       { radius: (4 / (Math.PI * 3))*100, step:  3, color:  "#0ff0f0",   angle: 0 },
       { radius: (4 / (Math.PI * 5))*100, step:  5, color:  "#00f00f",   angle: 0 },
       { radius: (4 / (Math.PI * 7))*100, step:  7, color:  "#000ff0",   angle: 0 },
       { radius: (4 / (Math.PI * 9))*100, step:  9, color:  "#0000ff",   angle: 0 },
       { radius: (4 / (Math.PI * 11))*100, step: 11, color:  "#0f0f00",   angle: 0 },
     ],
  centerX = 250,
  centerY = 200,
  redrawInternal = 30,
  curveDots = new Array(),
  curveLimit = 3000,
  curveColor = "black",

  log = function(x){
    console.log(x);
  },

  initCanvas = function(){
    canvas = document.getElementById('myCanvas');
    ctx = canvas.getContext('2d');
    intervalHandle = setInterval(draw, redrawInternal);
  },

  drawDotExt = function(x, y ,size){
  },

  drawDot = function(x, y){
    ctx.fillRect( x, y, 1, 1);
  },

  arrowLine = function(fromx, fromy, tox, toy){
    // http://stackoverflow.com/questions/808826/draw-arrow-on-canvas-tag
    var headlen = 10;   // length of head in pixels
    var angle = Math.atan2(toy-fromy,tox-fromx);
    ctx.moveTo(fromx, fromy);
    ctx.lineTo(tox, toy);
    ctx.lineTo(tox-headlen*Math.cos(angle-Math.PI/6),toy-headlen*Math.sin(angle-Math.PI/6));
    ctx.moveTo(tox, toy);
    ctx.lineTo(tox-headlen*Math.cos(angle+Math.PI/6),toy-headlen*Math.sin(angle+Math.PI/6));
  },

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

  drawCircle = function(cx, cy, radius, angle, color){
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

  updateModel = function(dotY) {
    for(var i = 0; i< cirMods.length; i++){
      cirMods[i].angle += cirMods[i].step;
      if(cirMods[i].angle == 360){
        cirMods[i].angle = 0;
      }
    }

    curveDots.push(dotY);
    if(curveDots.length > curveLimit){
      curveDots.shift();
    }
  },

  drawCurve = function(dot){
    var startX = centerX + 250;
    var startY = curveDots[curveDots.length-1];

    ctx.beginPath();
    arrowLine(dot.x, dot.y, startX, startY);
    drawDot(startX, startY);
    ctx.moveTo(startX, startY);
    for(var i = curveDots.length - 2; i >=0; i-- ) {
      ctx.lineTo(startX, curveDots[i]);
      startX++;
    }

    ctx.strokeStyle = curveColor;
    ctx.stroke();
  },

  draw = function(){
    var dot = {};

    clearRect();

    dot = drawCircle(centerX, centerY, cirMods[0].radius, cirMods[0].angle, cirMods[0].color);
    for(var i = 1; i< cirNum; i++){
      dot = drawCircle(dot.x, dot.y, cirMods[i].radius, cirMods[i].angle, cirMods[i].color);
    }

    updateModel(dot.y);

    drawCurve(dot);
  },

  updateClick = function(){
    cirNum = textbox.value;
    if(cirNum > 5) {
      cirNum = 5;
    }
    if(cirNum < 1) {
      cirNum = 1;
    }
    textBox.value = cirNum;
  },

  mainExec = function() {
    textbox = document.getElementById('txt0');
    document.getElementById('btn0').onclick = updateClick;
    textbox.value = cirNum;
    initCanvas();
    draw();
  };

  return {
    main: mainExec
  };

}());
