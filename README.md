# Dorm Elevator

Project 2 is centered around system calls, kernel programming, concurrency, synchronization, and an elevator scheduling algorithm. For the dorm elevator, we created a kernel module representing the elevator inside a building with passengers. It supports operations like starting, stopping, and issuing requests of those student passengers. The module also provides a "/proc/elevator" entry to display important elevator information.

## Group Members
- **Amelia Sieg**: ats20b@fsu.edu
- **Olivia Judah**: ogj21@fsu.edu
- **Rebecca Anestad**: rba20a@fsu.edu

## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: We created an empty C program named "empty" and then made a copy of it called "part1". We then added exactly four system calls to the "part1" program - write, sleep, fork, and exit. Then, we verified that we have added the correct number of system calls using the strace command.
- **Assigned to**: Amelia Sieg, Olivia Judah

### Part 2: Timer Kernel Module
- **Responsibilities**: We created a kernel module named "my_timer" that utilizes the function ktime_get_real_ts64() to retrieve the time value, which includes seconds and nanoseconds since the Unix Epoch. When the my_timer module is loaded using insmod, it creates a proc entry named "/proc/timer". When the my_timer module is unloaded using rmmod, the /proc/timer entry is removed. On each read operation of "/proc/timer", we used the proc interface to print both the current time and the elapsed time since the last call.
- **Assigned to**: Rebecca Anestad, Amelia Sieg

### Part 3a: Adding System Calls
- **Responsibilities**: We modified the kernel by adding three system calls (start_elevator, stop_elevator, and issue_request) to control the elevator and create passengers. We had to modify multiple files to incorproate the functionality of these system calls.
- **Assigned to**: Olivia Judah, Amelia Sieg

### Part 3b: Kernel Compilation
- **Responsibilities**: We compiled the kernel with our newly added system calls. It took a while and we also had to check that we installed our kernel by doing the "uname -r" command in the terminal.
- **Assigned to**: Amelia Sieg, Olivia Judah, Rebecca Anestad

### Part 3c: Threads
- **Responsibilities**: We use a kthread to control the elevator movement.
- **Assigned to**: Olivia Judah, Rebecca Anestad

### Part 3d: Linked List
- **Responsibilities**: We use linked lists to handle the number of passengers per floor/elevator.
- **Assigned to**: Rebecca Anestad, Amelia Sieg

### Part 3e: Mutexes
- **Responsibilities**: We use a mutex to control shared data access between floor and elevators. We had to ensure that we were locking and unlocking around all necessary information to prevent deadlocks.
- **Assigned to**: Amelia Sieg, Olivia Judah

### Part 3f: Scheduling Algorithm
- **Responsibilities**: Our scheduling algorithm is focused around the Shortest Seek Time First (SSTF) algorithm. SSTF selects the request with the least seek time from the current head position. In this case, our elevator scans the destination floors of its current passengers and goes to the floor that is the closest.
- **Assigned to**: Rebecca Anestad, Amelia Sieg, Olivia Judah

## File Listing
```
elevator/
├── Makefile
├── Part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   └── part1.trace
├── Part2/
│   ├── src/
|   |   ├── my_timer.c
│   └── Makefile
├── Part3/
│   ├── src/
|   |   ├── elevator.c
│   ├── Makefile
│   └── syscalls.c
└── README.md

```
### Requirements
- **Compiler**: `gcc` for C/C++

## Part 1
### Compilation
gcc -o empty empty.c
gcc -o part1 part1.c

This will build the executable in the same folder

### Execution
strace -o empty.trace ./empty
strace -o part1.trace ./part1

This will trace the programs

## Part 2
### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in the src folder in Part2

### Execution
sudo insmod my_timer.ko
This will run the kernel

watch -n 1 cat /proc/timer

sleep [number of seconds]

sudo rmmod my_timer.ko


## Part 3
### Compilation
make

This will build the executable in the src folder in Part3

### Execution
sudo insmod elevator.ko

This will run the kernel

watch -n 1 cat /proc/elevator
./producer [number of passengers]
./consumer --start
./consumer --stop

sudo rmmod elevator.ko

## Bugs
- No bugs to report

## Considerations
[None]
