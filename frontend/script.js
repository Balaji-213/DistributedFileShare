// Global variables
let currentUser = null;
let sessionToken = null;
let currentFileId = null;

// API Base URL
const API_BASE = 'http://localhost:8080';

// Check if user is already logged in
function checkSession() {
    const savedToken = localStorage.getItem('sessionToken');
    const savedUser = localStorage.getItem('currentUser');
    
    if (savedToken && savedUser) {
        sessionToken = savedToken;
        currentUser = JSON.parse(savedUser);
        showDashboard();
        loadFiles();
    } else {
        showLogin();
    }
}

// Setup event listeners
function setupEventListeners() {
    // Auth forms
    document.getElementById('login-form').addEventListener('submit', handleLogin);
    document.getElementById('register-form').addEventListener('submit', handleRegister);
    
    // File upload
    const fileInput = document.getElementById('file-input');
    const uploadArea = document.getElementById('upload-area');
    
    fileInput.addEventListener('change', handleFileSelect);
    
    // Drag and drop
    uploadArea.addEventListener('dragover', handleDragOver);
    uploadArea.addEventListener('dragleave', handleDragLeave);
    uploadArea.addEventListener('drop', handleFileDrop);
    
    // Share type radio buttons
    document.querySelectorAll('input[name="share-type"]').forEach(radio => {
        radio.addEventListener('change', handleShareTypeChange);
    });
}

// Auth functions
function showLogin() {
    document.getElementById('login-page').classList.remove('hidden');
    document.getElementById('dashboard-page').classList.add('hidden');
    
    // Reset forms
    document.getElementById('login-form').reset();
    document.getElementById('register-form').reset();
    
    // Show login tab by default
    showLoginForm();
}

function showRegister() {
    document.getElementById('login-form').classList.add('hidden');
    document.getElementById('register-form').classList.remove('hidden');
    document.querySelector('.tab-btn.active').classList.remove('active');
    document.querySelectorAll('.tab-btn')[1].classList.add('active');
}

function showLoginForm() {
    document.getElementById('login-form').classList.remove('hidden');
    document.getElementById('register-form').classList.add('hidden');
    document.querySelector('.tab-btn.active').classList.remove('active');
    document.querySelectorAll('.tab-btn')[0].classList.add('active');
}

async function handleLogin(e) {
    e.preventDefault();
    
    const username = document.getElementById('login-username').value;
    const password = document.getElementById('login-password').value;
    
    showLoading(true);
    
    try {
        const response = await fetch(`${API_BASE}/login`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ username, password })
        });
        
        const data = await response.json();
        
        if (data.success) {
            sessionToken = data.session_token;
            currentUser = {
                id: data.user_id,
                username: data.username || username
            };
            
            // Save to localStorage
            localStorage.setItem('sessionToken', sessionToken);
            localStorage.setItem('currentUser', JSON.stringify(currentUser));
            
            showMessage('Login successful!', 'success');
            showDashboard();
            loadFiles();
        } else {
            showMessage(data.error || 'Login failed', 'error');
        }
    } catch (error) {
        showMessage('Network error: ' + error.message, 'error');
    }
    
    showLoading(false);
}

async function handleRegister(e) {
    e.preventDefault();
    
    const username = document.getElementById('register-username').value.trim();
    const email = document.getElementById('register-email').value.trim();
    const password = document.getElementById('register-password').value;
    
    // ✅ ONLY CLIENT-SIDE VALIDATION (no real-time checking)
    if (username.length < 3) {
        showMessage('Username must be at least 3 characters long', 'error');
        return;
    }
    
    if (username.length > 50) {
        showMessage('Username must be less than 50 characters', 'error');
        return;
    }
    
    // Check for valid username characters
    const usernameRegex = /^[a-zA-Z0-9_-]+$/;
    if (!usernameRegex.test(username)) {
        showMessage('Username can only contain letters, numbers, underscores, and hyphens', 'error');
        return;
    }
    
    showLoading(true);
    
    try {
        const response = await fetch(`${API_BASE}/register`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ username, email, password })
        });
        
        const data = await response.json();
        
        if (data.success) {
            showMessage(`Registration successful! Your user ID is: ${data.user_id}. Please login.`, 'success');
            showLoginForm();
            document.getElementById('login-username').value = username;
        } else {
            // ✅ USERNAME UNIQUENESS CHECK HAPPENS HERE (on button click)
            if (response.status === 409) {
                showMessage(data.error, 'error'); // "Username already exists. Please choose a different username."
            } else {
                showMessage(data.error || 'Registration failed', 'error');
            }
        }
    } catch (error) {
        showMessage('Network error: ' + error.message, 'error');
    }
    
    showLoading(false);
}

// Call this in your DOMContentLoaded event
document.addEventListener('DOMContentLoaded', function() {
    setupEventListeners();
    checkSession();
});



function logout() {
    sessionToken = null;
    currentUser = null;
    localStorage.removeItem('sessionToken');
    localStorage.removeItem('currentUser');
    
    // Clear any displayed data
    document.getElementById('files-list').innerHTML = '';
    document.getElementById('shared-files-list').innerHTML = '';
    
    // Show success message before refresh
    showMessage('Logged out successfully', 'success');
    
    // Trigger page refresh after a short delay to show the message
    setTimeout(() => {
        window.location.reload();
    }, 1000); // 1 second delay to show the success message
}

// File upload functions
function handleFileSelect(e) {
    const files = Array.from(e.target.files);
    uploadFiles(files);
}

function handleDragOver(e) {
    e.preventDefault();
    document.getElementById('upload-area').classList.add('drag-over');
}

function handleDragLeave(e) {
    e.preventDefault();
    document.getElementById('upload-area').classList.remove('drag-over');
}

function handleFileDrop(e) {
    e.preventDefault();
    document.getElementById('upload-area').classList.remove('drag-over');
    
    const files = Array.from(e.dataTransfer.files);
    uploadFiles(files);
}

async function uploadFiles(files) {
    if (files.length === 0) return;
    
    const progressContainer = document.getElementById('upload-progress');
    const progressFill = document.getElementById('progress-fill');
    const uploadStatus = document.getElementById('upload-status');
    
    progressContainer.classList.remove('hidden');
    
    for (let i = 0; i < files.length; i++) {
        const file = files[i];
        const progress = ((i + 1) / files.length) * 100;
        
        progressFill.style.width = `${progress}%`;
        uploadStatus.textContent = `Uploading ${file.name}... (${i + 1}/${files.length})`;
        
        try {
            const response = await fetch(`${API_BASE}/upload`, {
                method: 'POST',
                headers: {
                    'Authorization': `Bearer ${sessionToken}`,
                    'X-Filename': file.name,
                    'Content-Type': file.type || 'application/octet-stream'
                },
                body: file
            });
            
            const data = await response.json();
            
            if (!data.success) {
                showMessage(`Failed to upload ${file.name}: ${data.error}`, 'error');
            }
        } catch (error) {
            showMessage(`Failed to upload ${file.name}: ${error.message}`, 'error');
        }
    }
    
    progressContainer.classList.add('hidden');
    showMessage(`Successfully uploaded ${files.length} file(s)`, 'success');
    loadFiles();
    
    // Reset file input
    document.getElementById('file-input').value = '';
}

// File management functions
async function loadFiles() {
    showLoading(true);
    
    try {
        const response = await fetch(`${API_BASE}/files`, {
            headers: {
                'Authorization': `Bearer ${sessionToken}`
            }
        });
        
        const data = await response.json();
        
        if (data.success) {
            displayFiles(data.files);
        } else {
            showMessage('Failed to load files: ' + data.error, 'error');
        }
    } catch (error) {
        showMessage('Network error: ' + error.message, 'error');
    }
    
    showLoading(false);
}

function displayFiles(files) {
    const filesList = document.getElementById('files-list');
    
    if (files.length === 0) {
        filesList.innerHTML = `
            <div style="text-align: center; padding: 40px; color: #666;">
                <i class="fas fa-folder-open" style="font-size: 3em; margin-bottom: 20px;"></i>
                <p>No files uploaded yet. Upload your first file above!</p>
            </div>
        `;
        return;
    }
    
    filesList.innerHTML = files.map(file => `
        <div class="file-item">
            <div class="file-icon">
                <i class="${getFileIcon(file.content_type)}"></i>
            </div>
            <div class="file-info">
                <div class="file-name">${file.filename}</div>
                <div class="file-details">
                    ${formatFileSize(file.size)} • ${file.content_type} • ${new Date(file.upload_date).toLocaleDateString()}
                </div>
            </div>
            <div class="file-actions">
                <button class="btn btn-primary btn-small" onclick="downloadFile(${file.file_id})">
                    <i class="fas fa-download"></i> Download
                </button>
                <button class="btn btn-secondary btn-small" onclick="openShareModal(${file.file_id}, '${file.filename}')">
                    <i class="fas fa-share"></i> Share
                </button>
            </div>
        </div>
    `).join('');
}

function getFileIcon(contentType) {
    if (contentType.startsWith('image/')) return 'fas fa-image';
    if (contentType.startsWith('video/')) return 'fas fa-video';
    if (contentType.startsWith('audio/')) return 'fas fa-music';
    if (contentType.includes('pdf')) return 'fas fa-file-pdf';
    if (contentType.includes('word')) return 'fas fa-file-word';
    if (contentType.includes('excel') || contentType.includes('spreadsheet')) return 'fas fa-file-excel';
    if (contentType.includes('powerpoint') || contentType.includes('presentation')) return 'fas fa-file-powerpoint';
    if (contentType.startsWith('text/')) return 'fas fa-file-alt';
    return 'fas fa-file';
}

function formatFileSize(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

async function downloadFile(fileId) {
    try {
        const response = await fetch(`${API_BASE}/download/${fileId}`, {
            headers: {
                'Authorization': `Bearer ${sessionToken}`
            }
        });
        
        if (response.ok) {
            const blob = await response.blob();
            const filename = response.headers.get('Content-Disposition')?.split('filename=')[1]?.replace(/"/g, '') || `file_${fileId}`;
            
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
            document.body.removeChild(a);
            
            showMessage('File downloaded successfully', 'success');
        } else {
            const error = await response.json();
            showMessage('Download failed: ' + error.error, 'error');
        }
    } catch (error) {
        showMessage('Download failed: ' + error.message, 'error');
    }
}

function refreshFiles() {
    loadFiles();
    showMessage('Files refreshed', 'success');
}

// Share modal functions
function openShareModal(fileId, filename) {
    currentFileId = fileId;
    document.getElementById('share-modal').classList.remove('hidden');
    document.getElementById('share-result').classList.add('hidden');
    document.getElementById('public-share').checked = true;
    document.getElementById('private-user-input').classList.add('hidden');
}

function closeShareModal() {
    document.getElementById('share-modal').classList.add('hidden');
    currentFileId = null;
}

function handleShareTypeChange(e) {
    const privateInput = document.getElementById('private-user-input');
    if (e.target.value === 'private') {
        privateInput.classList.remove('hidden');
    } else {
        privateInput.classList.add('hidden');
    }
}

async function generateShareLink() {
    if (!currentFileId) return;
    
    const shareType = document.querySelector('input[name="share-type"]:checked').value;
    const expiryHours = document.getElementById('expiry-hours').value;
    const sharedUserId = document.getElementById('shared-user-id').value;
    
    if (shareType === 'private' && !sharedUserId) {
        showMessage('Please enter a user ID for private sharing', 'error');
        return;
    }
    
    const generateBtn = document.getElementById('generate-share-btn');
    generateBtn.disabled = true;
    generateBtn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Generating...';
    
    try {
        const payload = {
            file_id: currentFileId,
            expiry_hours: expiryHours
        };
        
        if (shareType === 'private') {
            payload.shared_with_user_id = parseInt(sharedUserId);
        }
        
        const response = await fetch(`${API_BASE}/share`, {
            method: 'POST',
            headers: {
                'Authorization': `Bearer ${sessionToken}`,
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(payload)
        });
        
        const data = await response.json();
        
        if (data.success) {
            document.getElementById('share-url').value = data.share_url;
            document.getElementById('share-result').classList.remove('hidden');
            showMessage('Share link generated successfully!', 'success');
        } else {
            showMessage('Failed to generate share link: ' + data.error, 'error');
        }
    } catch (error) {
        showMessage('Failed to generate share link: ' + error.message, 'error');
    }
    
    generateBtn.disabled = false;
    generateBtn.innerHTML = '<i class="fas fa-link"></i> Generate Link';
}

function copyToClipboard() {
    const shareUrlInput = document.getElementById('share-url');
    shareUrlInput.select();
    shareUrlInput.setSelectionRange(0, 99999); // For mobile devices
    
    try {
        document.execCommand('copy');
        showMessage('Share link copied to clipboard!', 'success');
    } catch (err) {
        // Fallback for modern browsers
        navigator.clipboard.writeText(shareUrlInput.value).then(() => {
            showMessage('Share link copied to clipboard!', 'success');
        }).catch(() => {
            showMessage('Failed to copy link. Please copy manually.', 'error');
        });
    }
}

// Load files shared with current user
async function loadSharedFiles() {
    showLoading(true);
    
    try {
        const response = await fetch(`${API_BASE}/shared-with-me`, {
            headers: {
                'Authorization': `Bearer ${sessionToken}`
            }
        });
        
        const data = await response.json();
        
        if (data.success) {
            displaySharedFiles(data.shared_files);
        } else {
            showMessage('Failed to load shared files: ' + data.error, 'error');
        }
    } catch (error) {
        showMessage('Network error: ' + error.message, 'error');
    }
    
    showLoading(false);
}

function displaySharedFiles(files) {
    const sharedFilesList = document.getElementById('shared-files-list');
    
    if (files.length === 0) {
        sharedFilesList.innerHTML = `
            <div style="text-align: center; padding: 40px; color: #666;">
                <i class="fas fa-share-alt" style="font-size: 3em; margin-bottom: 20px;"></i>
                <p>No files have been shared with you yet.</p>
            </div>
        `;
        return;
    }
    
    sharedFilesList.innerHTML = files.map(file => `
        <div class="file-item shared-file-item">
            <div class="file-icon">
                <i class="${getFileIcon(file.content_type)}"></i>
            </div>
            <div class="file-info">
                <div class="file-name">${file.filename}</div>
                <div class="file-details">
                    ${formatFileSize(file.size)} • ${file.content_type} • Shared by: ${file.shared_by}
                    <br><small>Expires: ${file.expires_at ? new Date(file.expires_at).toLocaleString() : 'Never'}</small>
                </div>
            </div>
            <div class="file-actions">
                <button class="btn btn-primary btn-small" onclick="downloadSharedFile('${file.share_token}', '${file.filename}')">
                    <i class="fas fa-download"></i> Download
                </button>
                <button class="btn btn-secondary btn-small" onclick="downloadFile(${file.file_id})">
                    <i class="fas fa-external-link-alt"></i> Direct Download
                </button>
            </div>
        </div>
    `).join('');
}

// Download shared file using share token
async function downloadSharedFile(shareToken, filename) {
    try {
        const response = await fetch(`${API_BASE}/shared/${shareToken}`, {
            headers: {
                'Authorization': `Bearer ${sessionToken}`
            }
        });
        
        if (response.ok) {
            const blob = await response.blob();
            
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
            document.body.removeChild(a);
            
            showMessage('Shared file downloaded successfully', 'success');
        } else {
            const error = await response.json();
            showMessage('Download failed: ' + error.error, 'error');
        }
    } catch (error) {
        showMessage('Download failed: ' + error.message, 'error');
    }
}

function refreshSharedFiles() {
    loadSharedFiles();
    showMessage('Shared files refreshed', 'success');
}

// Update showDashboard function to load shared files
function showDashboard() {
    document.getElementById('login-page').classList.add('hidden');
    document.getElementById('dashboard-page').classList.remove('hidden');
    
    // Display detailed user info
    document.getElementById('username-display').textContent = `Welcome, ${currentUser.username} | User ID: ${currentUser.id}`;
    
    loadFiles();
    loadSharedFiles();
}


// Utility functions
function showLoading(show) {
    const loading = document.getElementById('loading');
    if (show) {
        loading.classList.remove('hidden');
    } else {
        loading.classList.add('hidden');
    }
}

function showMessage(message, type) {
    const messageContainer = document.getElementById('message-container');
    
    const messageElement = document.createElement('div');
    messageElement.className = `message ${type}`;
    messageElement.innerHTML = `
        <strong>${type === 'success' ? 'Success!' : 'Error!'}</strong> ${message}
    `;
    
    messageContainer.appendChild(messageElement);
    
    // Auto remove after 5 seconds
    setTimeout(() => {
        if (messageElement.parentNode) {
            messageElement.parentNode.removeChild(messageElement);
        }
    }, 5000);
}

// Tab switching for auth
window.showLogin = showLoginForm;
window.showRegister = showRegister;
