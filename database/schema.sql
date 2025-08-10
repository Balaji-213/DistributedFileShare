-- Create database
CREATE DATABASE IF NOT EXISTS fileshare;
\c fileshare;

-- Users table
CREATE TABLE IF NOT EXISTS users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(100) UNIQUE NOT NULL,
    password_hash CHAR(64) NOT NULL,
    email VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Files table
CREATE TABLE IF NOT EXISTS files (
    file_id SERIAL PRIMARY KEY,
    filename VARCHAR(255) NOT NULL,
    original_filename VARCHAR(255) NOT NULL,
    file_path VARCHAR(500) NOT NULL,
    file_size BIGINT NOT NULL,
    content_type VARCHAR(100),
    owner_id INTEGER REFERENCES users(user_id),
    upload_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_public BOOLEAN DEFAULT FALSE
);

-- File shares table
CREATE TABLE IF NOT EXISTS file_shares (
    share_id SERIAL PRIMARY KEY,
    file_id INTEGER REFERENCES files(file_id),
    shared_by INTEGER REFERENCES users(user_id),
    shared_with INTEGER REFERENCES users(user_id),
    share_token VARCHAR(64) UNIQUE,
    expires_at TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Sessions table
CREATE TABLE IF NOT EXISTS user_sessions (
    session_id VARCHAR(64) PRIMARY KEY,
    user_id INTEGER REFERENCES users(user_id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP
);

-- Indexes for performance
CREATE INDEX IF NOT EXISTS idx_files_owner ON files(owner_id);
CREATE INDEX IF NOT EXISTS idx_shares_file ON file_shares(file_id);
CREATE INDEX IF NOT EXISTS idx_shares_token ON file_shares(share_token);
CREATE INDEX IF NOT EXISTS idx_sessions_user ON user_sessions(user_id);