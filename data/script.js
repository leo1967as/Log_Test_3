document.addEventListener('DOMContentLoaded', () => {
    // ส่วนควบคุมสถานะ
    const startBtn = document.getElementById('startBtn');
    const stopBtn = document.getElementById('stopBtn');
    const pauseBtn = document.getElementById('pauseBtn');
    const statusEl = document.getElementById('status');
    const fileNameEl = document.getElementById('fileName');

    // ส่วนแสดงผลข้อมูล
    const dataFlowEl = document.getElementById('data-flow');
    const dataWeightEl = document.getElementById('data-weight');

    // ฟังก์ชันส่งคำสั่งควบคุม (Start/Stop/Pause/Status)
    function sendCommand(command) {
        fetch('/control', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `action=${command}`
        })
        .then(response => response.json())
        .then(data => {
            statusEl.textContent = data.status;
            // แสดงชื่อไฟล์เมื่อมีการบันทึก หรือเมื่อสถานะเป็น Paused
            if (data.status === 'Recording' || data.status === 'Paused') {
                fileNameEl.textContent = data.fileName;
            } else {
                fileNameEl.textContent = 'N/A';
            }
            console.log(data.message);
        })
        .catch(error => console.error('Error sending command:', error));
    }

    // ฟังก์ชันดึงข้อมูลล่าสุดจาก ESP8266
    function fetchData() {
        fetch('/data')
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
                return response.json();
            })
            .then(data => {
                // อัปเดตส่วนแสดงผลข้อมูล
                dataFlowEl.textContent = data.flowrate.toFixed(2);
                dataWeightEl.textContent = data.weight.toFixed(2);
            })
            .catch(error => {
                console.error('Error fetching data:', error);
                // แสดงสถานะว่ามีปัญหาในการเชื่อมต่อ
                dataFlowEl.textContent = 'Err';
                dataWeightEl.textContent = 'Err';
            });
    }

    // --- Event Listeners ---
    startBtn.addEventListener('click', () => sendCommand('start'));
    stopBtn.addEventListener('click', () => sendCommand('stop'));
    pauseBtn.addEventListener('click', () => sendCommand('pause'));

    // --- เริ่มการทำงาน ---
    // 1. ดึงสถานะปัจจุบันเมื่อโหลดหน้าเว็บ
    sendCommand('status'); 
    
    // 2. เริ่มดึงข้อมูล Real-time ทุกๆ 2 วินาที
    setInterval(fetchData, 2000); 
    
    // 3. ดึงสถานะปัจจุบันทุกๆ 5 วินาที เพื่อให้ข้อมูลตรงกันเสมอ
    setInterval(() => sendCommand('status'), 5000);
});