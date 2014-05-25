My = {}

function myMain()
{
  initCanvas();
  drawSomething();
}

function initCanvas()
{
  My.canvas = document.getElementById('myCanvas');
  My.context = My.canvas.getContext('2d');
}

function dot( x, y ) 
{
  var ctx = My.context;
  ctx.fillRect( x, y, 1, 1);
}

function drawSomething()
{
  var ctx = My.context;
  var i = 0;
  var x, y, len, x1, y1
  
  
  i = 0;
  while( i < 361 ) {
    x = i;
    y = Math.sin( x * (Math.PI / 180)  ) * 60;
    dot( x/2, y + 100 );
    i ++
  }
  console.log("draw sin");
  
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
  console.log("draw circle");
} 