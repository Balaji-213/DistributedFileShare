# Testing Guide - Distributed File Sharing System

## Test Environment Setup

- Backend server: `http://localhost:8080`
- Frontend server: `http://localhost:3000`
- YugabyteDB: `localhost:5433`
- curl for API testing

## Backend API Tests

### 1. User Registration
curl -X POST http://localhost:8080/register
-H "Content-Type: application/json"
-d '{"username": "testuser1", "password": "password123"}'

text

### 2. User Authentication
curl -X POST http://localhost:8080/login
-H "Content-Type: application/json"
-d '{"username": "testuser1", "password": "password123"}'

Save session_token from result
text

### 3. File Upload
echo "test file content" > test.txt
curl -X POST http://localhost:8080/upload
-H "Authorization: Bearer YOUR_SESSION_TOKEN"
-H "X-Filename: test.txt"
--data-binary @test.txt

text

### 4. File Operations
List files
curl -X GET http://localhost:8080/files
-H "Authorization: Bearer YOUR_SESSION_TOKEN"

Download file
curl -X GET http://localhost:8080/download/1
-H "Authorization: Bearer YOUR_SESSION_TOKEN"
-o downloaded.txt

text

### 5. File Sharing
Public share
curl -X POST http://localhost:8080/share
-H "Authorization: Bearer YOUR_SESSION_TOKEN"
-H "Content-Type: application/json"
-d '{"file_id": 1, "expiry_hours": "24"}'

text

## Database Testing
- Check all users: `SELECT * FROM users;`
- Check all files: `SELECT * FROM files;`
- Check all shares: `SELECT * FROM file_shares;`

## Frontend Testing Checklist

- Register user
- Login with correct credentials and see dashboard
- Upload/download files
- Share public and private links
- Logout and verify session ends
- Proper error handling for all cases

## Troubleshooting
- Check browser console for errors (F12)
- Check server output/logs for any backend errors