document.addEventListener('DOMContentLoaded', () => {
    const startBtn = document.getElementById('startBtn');
    const stopBtn = document.getElementById('stopBtn');
    const pauseBtn = document.getElementById('pauseBtn');
    const statusEl = document.getElementById('status');

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

    startBtn.addEventListener('click', () => sendCommand('start'));
    stopBtn.addEventListener('click', () => sendCommand('stop'));
    pauseBtn.addEventListener('click', () => sendCommand('pause'));

    // Fetch initial status on load
    sendCommand('status');
});