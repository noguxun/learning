var drawingApp = (function(){
  "use strict";

  var canvas,
  
  context,

  // Initial parameter setting
  gravity_mps2 =9.80665, 
  initialAngle_rad = Math.PI*50/100,
  timestep_ms = 5,
  timestep_s = timestep_ms / 1000,
  
  penDus = [],
  penDusNum = 30,
  penDusStep = 0.07,
  
  
  updatePenDu = function(penDu) {
    var acceleration = penDu.k * Math.sin(penDu.angle);
    penDu.velocity += acceleration * timestep_s;
    penDu.angle    += penDu.velocity * timestep_s;
  },

  log = function(x){
    console.log(x);
  },

  initCanvas = function(){
    canvas = document.getElementById('myCanvas');
    context = canvas.getContext('2d');
    setInterval(draw, timestep_ms);
  },
  
  createPendulum = function(length_m) {
    var penDu = new Object();
    
    penDu.rPend = 200 * length_m;
    penDu.rBall = 5;
    penDu.rBar = 0.5;
    penDu.angle = initialAngle_rad;
    penDu.velocity = 0;
    penDu.k = -gravity_mps2/length_m;
    
    return penDu;
  },

  drawPendu = function(penDu) {

    updatePenDu(penDu);
    
    context.save();
      context.translate(canvas.width/2, 30);
      context.rotate(penDu.angle);
      
      
      context.beginPath();
      context.rect(-penDu.rBar, -penDu.rBar, penDu.rBar*2, penDu.rPend + penDu.rBar*2);
      context.fill();
      context.stroke(); 
 
      context.beginPath();
      context.arc(0, penDu.rPend, penDu.rBall, 0, Math.PI*2, false);
      context.fill();
      context.stroke();
    context.restore();
    
  },

  draw = function(){
    context.fillStyle = "rgba(255,255,255,0.51)";
    context.globalCompositeOperation = "destination-out";
    context.fillRect(0, 0, canvas.width, canvas.height);
 
    context.fillStyle = "yellow";
    context.globalCompositeOperation = "source-over";
    
    for (var i = 0; i < penDusNum; i++) {
      drawPendu(penDus[i]);
    }
  },

  mainExec = function() {
    for (var i = 0; i < penDusNum; i++) {
      penDus[i] = createPendulum(0.8 + penDusStep * i);
    }
    
    initCanvas();
    draw();
  };

  return {
    main: mainExec
  };

}());
