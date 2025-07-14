document.addEventListener('DOMContentLoaded', () => {
    // ส่วนควบคุมสถานะ
    const startBtn = document.getElementById('startBtn');
    const stopBtn = document.getElementById('stopBtn');
    const pauseBtn = document.getElementById('pauseBtn');
    const statusEl = document.getElementById('status');

    // ส่วนแสดงผลข้อมูล (ใหม่)
    const dataFlowEl = document.getElementById('data-flow');
    const dataTempEl = document.getElementById('data-temp');
    const dataTimeEl = document.getElementById('data-time');

    // ฟังก์ชันส่งคำสั่งควบคุม (Start/Stop/Pause)
    function sendCommand(command) {
        fetch('/control', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `action=${command}`
        })
        .then(response => response.json())
        .then(data => {
            statusEl.textContent = data.status;
            console.log(data.message);
        })
        .catch(error => console.error('Error:', error));
    }

    // ฟังก์ชันดึงข้อมูลล่าสุดจาก ESP8266 (ใหม่)
    function fetchData() {
        fetch('/data')
            .then(response => response.json())
            .then(data => {
                // อัปเดตส่วนแสดงผลข้อมูล
                dataFlowEl.textContent = data.flowrate.toFixed(2);
                dataTempEl.textContent = data.temp.toFixed(2);
                // แปลง timestamp (millis) เป็นเวลาที่อ่านง่าย
                dataTimeEl.textContent = new Date(data.timestamp).toLocaleTimeString();
            })
            .catch(error => console.error('Error fetching data:', error));
    }

    // --- Event Listeners ---
    startBtn.addEventListener('click', () => sendCommand('start'));
    stopBtn.addEventListener('click', () => sendCommand('stop'));
    pauseBtn.addEventListener('click', () => sendCommand('pause'));

    // --- เริ่มการทำงาน ---
    sendCommand('status'); // ดึงสถานะเริ่มต้นเมื่อโหลดหน้า
    setInterval(fetchData, 2000); // ดึงข้อมูลใหม่ๆ ทุกๆ 2 วินาที
});