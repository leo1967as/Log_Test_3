document.addEventListener('DOMContentLoaded', () => {
    const fileList = document.getElementById('fileList');

    function loadFiles() {
        fetch('/list-csv')
            .then(response => response.json())
            .then(files => {
                fileList.innerHTML = ''; // Clear existing list
                if (files.length === 0) {
                    fileList.innerHTML = '<li>No CSV files found.</li>';
                } else {
                    files.forEach(file => {
                        const li = document.createElement('li');
                        li.innerHTML = `
                            <span class="fileName">${file.name}</span>
                            <div class="file-actions">
                                <a href="/download?file=${file.name}" class="download-btn">Download</a>
                                <button class="delete-btn" data-filename="${file.name}">Delete</button>
                            </div>
                        `;
                        fileList.appendChild(li);
                    });
                }
            })
            .catch(error => console.error('Error fetching file list:', error));
    }

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

    loadFiles();
});