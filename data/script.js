// Global variables
let radarActive = false;
let radarData = [];
let sweepAngle = 90; // Start at center (90°)
let canvas, ctx;

// Initialize when page loads
document.addEventListener('DOMContentLoaded', function() {
    // Initialize slider for square movement
    const slider = document.getElementById('square-size');
    const sizeValue = document.getElementById('size-value');
    
    slider.addEventListener('input', function() {
        sizeValue.textContent = this.value;
    });
    
    // Initialize radar canvas
    canvas = document.getElementById('radarCanvas');
    if (canvas) {
        ctx = canvas.getContext('2d');
        drawRadar(); // Draw initial radar display
    }
});

// Motor control functions
function sendCmd(command) {
    fetch('/cmd?val=' + command)
        .then(response => response.text())
        .then(data => {
            console.log('Command sent:', command);
            updateRobotStatus(command);
        })
        .catch(error => console.error('Error:', error));
}

function updateRobotStatus(command) {
    const statusElement = document.getElementById('robot-current-status');
    switch(command) {
        case 'F': statusElement.textContent = 'Moving Forward'; break;
        case 'G': statusElement.textContent = 'Moving Backward'; break;
        case 'L': statusElement.textContent = 'Turning Left'; break;
        case 'R': statusElement.textContent = 'Turning Right'; break;
        case 'S': statusElement.textContent = 'Stopped'; break;
        default: statusElement.textContent = 'Unknown'; break;
    }
}

// Square movement function
function moveSquare() {
    const size = document.getElementById('square-size').value;
    fetch('/square?size=' + size)
        .then(response => response.text())
        .then(data => {
            console.log('Square movement started:', data);
            document.getElementById('robot-current-status').textContent = 'Moving in Square';
        })
        .catch(error => console.error('Error:', error));
}

// Radar control functions
function startRadar() {
    console.log('Starting radar sweep...');
    radarActive = true;
    
    // Start radar mode on server (servo will auto-sweep 0->180->0)
    fetch('/radar?mode=start')
        .then(response => response.text())
        .then(data => {
            console.log('Radar started:', data);
            document.getElementById('object-status').textContent = 'Radar Sweeping...';
            document.getElementById('object-status').style.color = '#00ff00';
            
            // Start fetching radar data
            fetchRadarData();
        })
        .catch(error => {
            console.error('Error starting radar:', error);
            document.getElementById('object-status').textContent = 'Start Error';
            document.getElementById('object-status').style.color = '#ff0000';
        });
}

function stopRadar() {
    console.log('Stopping radar...');
    radarActive = false;
    
    // Stop radar mode on server (servo will move to center 90°)
    fetch('/radar?mode=stop')
        .then(response => response.text())
        .then(data => {
            console.log('Radar stopped:', data);
            document.getElementById('object-status').textContent = 'Radar Stopped';
            document.getElementById('object-status').style.color = '#888888';
            
            // Reset to center position
            sweepAngle = 90;
            document.getElementById('angle-display').textContent = 'Servo: 90° | Radar: 0°';
            
            // Redraw radar
            if (canvas && ctx) {
                drawRadar();
            }
        })
        .catch(error => {
            console.error('Error stopping radar:', error);
            document.getElementById('object-status').textContent = 'Stop Error';
            document.getElementById('object-status').style.color = '#ff0000';
        });
}

function clearRadarData() {
    radarData = [];
    if (canvas && ctx) {
        drawRadar();
    }
    console.log('Radar data cleared');
}

// Fetch radar data from server
function fetchRadarData() {
    if (!radarActive) return;
    
    fetch('/radar-data')
        .then(response => response.json())
        .then(data => {
            console.log('Radar data received:', data);
            
            // Convert servo angle to radar angle
            const radarAngle = data.angle - 90; // 0° servo = -90° radar, 180° servo = +90° radar
            document.getElementById('angle-display').textContent = `Servo: ${data.angle}° | Radar: ${radarAngle}°`;
            document.getElementById('distance-display').textContent = `Distance: ${data.distance} cm`;
            
            // Update object status
            const objectStatus = document.getElementById('object-status');
            if (data.distance > 0 && data.distance < 30) {
                objectStatus.textContent = 'Object Detected!';
                objectStatus.style.color = '#ff0000';
            } else if (data.distance > 0) {
                objectStatus.textContent = 'Radar Sweeping...';
                objectStatus.style.color = '#00ff00';
            } else {
                objectStatus.textContent = 'Sensor Error';
                objectStatus.style.color = '#ffff00';
            }
            
            // Add valid data to radar array
            if (data.distance > 0) {
                radarData.push({
                    angle: data.angle,
                    distance: data.distance
                });
                
                // Keep only last 180 points (one full sweep)
                if (radarData.length > 180) {
                    radarData.shift();
                }
            }
            
            // Update sweep angle
            sweepAngle = data.angle;
            
            // Redraw radar
            drawRadar();
            
            // Continue fetching if radar is active
            setTimeout(fetchRadarData, 100); // Fetch every 100ms
        })
        .catch(error => {
            console.error('Error fetching radar data:', error);
            document.getElementById('object-status').textContent = 'Connection Error';
            document.getElementById('object-status').style.color = '#ff0000';
            
            // Retry after 1 second if radar is still active
            if (radarActive) {
                setTimeout(fetchRadarData, 1000);
            }
        });
}

// Draw radar display
function drawRadar() {
    if (!canvas || !ctx) return;
    
    // Clear canvas
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const maxRadius = Math.min(centerX, centerY) - 20;
    
    // Set styles
    ctx.strokeStyle = '#00ff00';
    ctx.fillStyle = '#001100';
    ctx.lineWidth = 1;
    
    // Fill background
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    
    // Draw concentric circles (radar rings)
    for (let i = 1; i <= 4; i++) {
        ctx.beginPath();
        ctx.arc(centerX, centerY, (maxRadius / 4) * i, 0, 2 * Math.PI);
        ctx.stroke();
    }
    
    // Draw angle lines (9h-3h orientation: -90° to +90°)
    ctx.strokeStyle = '#00ff00';
    ctx.lineWidth = 1;
    
    for (let angle = -90; angle <= 90; angle += 30) {
        const radian = (angle * Math.PI) / 180;
        const x = centerX + maxRadius * Math.cos(radian);
        const y = centerY + maxRadius * Math.sin(radian);
        
        ctx.beginPath();
        ctx.moveTo(centerX, centerY);
        ctx.lineTo(x, y);
        ctx.stroke();
        
        // Draw angle labels
        const labelX = centerX + (maxRadius + 15) * Math.cos(radian);
        const labelY = centerY + (maxRadius + 15) * Math.sin(radian);
        
        ctx.fillStyle = '#00ff00';
        ctx.font = '10px Arial';
        ctx.textAlign = 'center';
        ctx.fillText(angle + '°', labelX, labelY);
    }
    
    // Draw center point
    ctx.fillStyle = '#ff0000';
    ctx.beginPath();
    ctx.arc(centerX, centerY, 3, 0, 2 * Math.PI);
    ctx.fill();
    
    // Draw range labels
    ctx.fillStyle = '#00ff00';
    ctx.font = '10px Arial';
    ctx.textAlign = 'center';
    
    for (let i = 1; i <= 4; i++) {
        const radius = (maxRadius / 4) * i;
        const distance = (i * 100); // Max range 400cm
        ctx.fillText(distance + 'cm', centerX + radius, centerY - 5);
    }
    
    // Draw radar data points
    drawRadarData();
    
    // Draw sweep line
    drawSweepLine();
}

// Draw radar data points
function drawRadarData() {
    if (!canvas || !ctx) return;
    
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const maxRadius = Math.min(centerX, centerY) - 20;
    
    radarData.forEach(data => {
        // Convert servo angle (0-180°) to radar angle (-90° to +90°)
        const radarAngle = data.angle - 90;
        const radian = (radarAngle * Math.PI) / 180;
        
        // Calculate point position based on distance
        const distance = Math.min(data.distance, 400);
        const pointRadius = (distance / 400) * maxRadius;
        
        const x = centerX + pointRadius * Math.cos(radian);
        const y = centerY + pointRadius * Math.sin(radian);
        
        // Draw object point
        ctx.fillStyle = '#ff0000';
        ctx.beginPath();
        ctx.arc(x, y, 3, 0, 2 * Math.PI);
        ctx.fill();
        
        // Draw distance text
        ctx.fillStyle = '#ffff00';
        ctx.font = '8px Arial';
        ctx.textAlign = 'center';
        ctx.fillText(data.distance.toFixed(0) + 'cm', x, y - 8);
    });
}

// Draw sweep line
function drawSweepLine() {
    if (!canvas || !ctx) return;
    
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const maxRadius = Math.min(centerX, centerY) - 20;
    
    // Convert servo angle to radar angle
    const radarAngle = sweepAngle - 90;
    const radian = (radarAngle * Math.PI) / 180;
    
    const x = centerX + maxRadius * Math.cos(radian);
    const y = centerY + maxRadius * Math.sin(radian);
    
    // Draw sweep line
    ctx.strokeStyle = '#00ff00';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(centerX, centerY);
    ctx.lineTo(x, y);
    ctx.stroke();
    
    // Draw fade effect for sweep
    const gradient = ctx.createRadialGradient(centerX, centerY, 0, centerX, centerY, maxRadius);
    gradient.addColorStop(0, 'rgba(0, 255, 0, 0.3)');
    gradient.addColorStop(1, 'rgba(0, 255, 0, 0)');
    
    ctx.fillStyle = gradient;
    ctx.beginPath();
    ctx.moveTo(centerX, centerY);
    ctx.arc(centerX, centerY, maxRadius, radian - 0.1, radian + 0.1);
    ctx.closePath();
    ctx.fill();
}