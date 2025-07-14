document.addEventListener('DOMContentLoaded', () => {
    const fileList = document.getElementById('fileList');
    const refreshBtn = document.getElementById('refreshBtn'); // Get the new button

    // Function to fetch and display the list of files
    function loadFiles() {
        // Show a loading message while fetching
        fileList.innerHTML = '<li>Loading file list...</li>';
        
        fetch('/list-csv')
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
                return response.json();
            })
            .then(files => {
                fileList.innerHTML = ''; // Clear existing list

                if (files.length === 0) {
                    fileList.innerHTML = '<li>No CSV files found on the SD card.</li>';
                    return;
                }
                
                files.forEach(fileName => {
                    const li = document.createElement('li');
                    li.innerHTML = `
                        <span class="fileName">${fileName}</span>
                        <div class="file-actions">
                            <a href="/download?file=${fileName}" class="download-btn">Download</a>
                            <button class="delete-btn" data-filename="${fileName}">Delete</button>
                        </div>
                    `;
                    fileList.appendChild(li);
                });
            })
            .catch(error => {
                console.error('Error fetching file list:', error);
                fileList.innerHTML = '<li>Error loading file list. Please check the connection and rescan.</li>';
            });
    }

    // --- NEW: Event listener for the refresh button ---
    refreshBtn.addEventListener('click', () => {
        // Disable the button and show feedback to the user
        refreshBtn.disabled = true;
        refreshBtn.textContent = 'Scanning...';
        fileList.innerHTML = '<li>Remounting SD card, please wait...</li>';

        // Step 1: Call the new /remount-sd endpoint
        fetch('/remount-sd', { method: 'POST' })
            .then(response => response.json())
            .then(data => {
                console.log(data.message); // Log message from ESP
                if (data.success) {
                    // Step 2: If remount was successful, load the files
                    loadFiles();
                } else {
                    // Show an error if remount failed
                    fileList.innerHTML = `<li style="color:red;">${data.message}</li>`;
                }
            })
            .catch(error => {
                console.error('Error remounting SD card:', error);
                fileList.innerHTML = '<li>Failed to communicate with the device.</li>';
            })
            .finally(() => {
                // Step 3: Re-enable the button after the process is complete
                refreshBtn.disabled = false;
                refreshBtn.textContent = 'Rescan SD Card';
            });
    });

    // Event listener for deleting files (Event Delegation)
    fileList.addEventListener('click', (event) => {
        if (event.target.classList.contains('delete-btn')) {
            const filename = event.target.getAttribute('data-filename');
            if (confirm(`Are you sure you want to delete ${filename}?`)) {
                fetch(`/delete?file=${filename}`, { method: 'DELETE' })
                    .then(response => response.json())
                    .then(data => {
                        alert(data.message);
                        if (data.success) {
                            loadFiles(); // Refresh the list
                        }
                    })
                    .catch(error => console.error('Error deleting file:', error));
            }
        }
    });

    // Initial load of files when the page is ready
    loadFiles();
});