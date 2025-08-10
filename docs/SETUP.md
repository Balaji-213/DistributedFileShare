# Distributed File Sharing System - Setup Guide

## Overview
A distributed file sharing system built with C++ backend (POCO libraries), HTML/CSS/JS frontend, and YugabyteDB database.

## System Requirements

### Hardware
- **RAM**: Minimum 4GB, Recommended 8GB+
- **Storage**: Minimum 10GB free space
- **CPU**: Multi-core processor recommended

### Operating System
- **Linux:** Ubuntu 18.04+ (Recommended)
- **Windows:** Windows 10+ with WSL2
- **macOS:** macOS 10.15+

## Prerequisites

### 1. YugabyteDB Installation
wget https://software.yugabyte.com/releases/2024.2.4.0/yugabyte-2024.2.4.0-b89-linux-x86_64.tar.gz 
tar -xzf yugabyte-2024.2.4.0-b89-linux-x86_64.tar.gz
mv yugabyte-2024.2.4.0 yugabyte
cd yugabyte/
./bin/yugabyted start 
Note: use stop it stop

Note: if status is stuck in Bootstraping use following comands for windows wsl
sudo locale-gen en_US.UTF-8
sudo update-locale LANG=en_US.UTF-8

### 2. POCO C++ Libraries Installation

**Ubuntu/Debian:**
sudo apt-get update
sudo apt-get install libpoco-dev libpoco-data-postgresql-dev libpoco-crypto-dev libpoco-json-dev


**CentOS/RHEL:**
sudo yum install poco-devel poco-data-postgresql-devel poco-crypto-devel poco-json-devel


### 3. Build Tools
sudo apt-get install build-essential cmake git


## Project Setup

### 1. Clone/Create Project Structure
mkdir DistributedFileShare
cd DistributedFileShare
mkdir -p src frontend database docs build uploads

### 2. Database Setup
Connect to YugabyteDB
./bin/ysqlsh -h 127.0.1.1 -p 5433 -U yugabyte
\i database/schema.sql


### 3. Backend Compilation
cd build
cmake ..
make -j4


### 4. Frontend Setup
cd frontend
python -m http.server 3000


## Running the System

1. **Start Database**
2. **Start Backend Server**
3. **Start Frontend Server**
4. **Access:** http://localhost:3000

## Troubleshooting

1. **CORS Errors:** Set headers in WebServer.cpp
2. **Database Connection:** Verify YugabyteDB is running
3. **Port Conflicts:** Change ports if 3000/8080 are used
4. **POCO Library Issues:** Check installation and paths

## Security Notes

- Change default DB passwords in production
- Use HTTPS in production
- Regular security updates for dependencies