MiniGit

A lightweight, educational version control system implemented in C. MiniGit provides core Git-like functionality including file staging, commits with hashing, commit history viewing, and checkout capabilities.

Features

Initialize Repository - Create a new .minigit repository
Stage Files - Add files to the staging area before committing
Commit Changes - Create snapshots with custom messages, timestamps, and parent references
View Commit Log - Browse the complete commit history
Checkout Commits - Restore files from any previous commit
View Status - Check staged files and current HEAD commit
How It Works

MiniGit uses:

Simple hashing (FNV-1a) for generating unique object IDs
Blob storage in .minigit/objects/ for file contents
Commit objects in .minigit/commits/ storing parent, timestamp, message, and file references
Index file (.minigit/index) to track staged files
HEAD file pointing to the current commit
Installation

Compile the program using any C compiler:

bash
gcc -o minigit minigit.c
Usage

Initialize a Repository

bash
./minigit init
Stage Files

bash
./minigit add filename.txt
Commit Changes

bash
./minigit commit -m "Your commit message"
View Commit History

bash
./minigit log
Checkout a Commit

bash
./minigit checkout <commit-id>
Check Status

bash
./minigit status
Display Help

bash
./minigit help
Example Workflow

bash
# Create a new repository
./minigit init

# Create a file and stage it
echo "Hello World" > hello.txt
./minigit add hello.txt

# Commit the file
./minigit commit -m "Initial commit"

# Make changes and commit again
echo "Second line" >> hello.txt
./minigit add hello.txt
./minigit commit -m "Update hello.txt"

# View history
./minigit log

# Restore a previous version
./minigit checkout <commit-id>
Directory Structure

After initialization, the repository contains:

text
.minigit/
├── objects/          # Blob storage (file snapshots)
├── commits/          # Commit objects (.commit files)
├── index            # Staging area
└── HEAD             # Current commit pointer
Limitations

No branch support (linear history only)
Simple hash algorithm (not cryptographic)
No remote operations (push/pull)
No file deletion handling
No diff functionality
Basic error handling
Educational Purpose

This project is designed for learning how version control systems work under the hood. It demonstrates core concepts like:

Content-addressable storage
Commit graphs
Staging areas
Object persistence

License

Feel free to use, modify, and distribute this code for educational purposes.
