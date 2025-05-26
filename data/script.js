function sendCmd(cmd) {
  // Gửi lệnh đến server
  fetch("/cmd?val=" + cmd)
    .then(response => console.log('Command sent: ' + cmd))
    .catch(error => console.error('Error:', error));
  
  // Cập nhật trạng thái hiển thị
  const statusElement = document.getElementById('robot-current-status');
  
  // Hiển thị trạng thái dựa trên lệnh
  switch(cmd) {
    case 'F':
      statusElement.textContent = 'Moving Forward';
      break;
    case 'G':
      statusElement.textContent = 'Moving Backward';
      break;
    case 'L':
      statusElement.textContent = 'Turning Left';
      break;
    case 'R':
      statusElement.textContent = 'Turning Right';
      break;
    case 'S':
      statusElement.textContent = 'Stopped';
      break;
    default:
      statusElement.textContent = 'Unknown Command';
  }
}

// Hàm điều khiển robot di chuyển theo hình vuông
function moveSquare() {
  const sizeSlider = document.getElementById('square-size');
  const size = sizeSlider.value;
  const statusElement = document.getElementById('robot-current-status');
  
  statusElement.textContent = 'Moving in Square Pattern';
  
  fetch("/square?size=" + size)
    .then(response => response.text())
    .then(data => {
      console.log('Square movement started:', data);
      setTimeout(() => {
        statusElement.textContent = 'Completed Square Movement';
      }, size * 4 * 50 + 3000); // Ước lượng thời gian hoàn thành
    })
    .catch(error => {
      console.error('Error:', error);
      statusElement.textContent = 'Error in Square Movement';
    });
}

// Cập nhật giá trị hiển thị khi slider thay đổi
document.addEventListener('DOMContentLoaded', function() {
  const sizeSlider = document.getElementById('square-size');
  const sizeValue = document.getElementById('size-value');
  
  if (sizeSlider && sizeValue) {
    sizeSlider.oninput = function() {
      sizeValue.textContent = this.value;
    };
  }
});