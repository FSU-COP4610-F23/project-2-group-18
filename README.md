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
- **Responsibilities**: [Description]
- **Assigned to**: Rebecca Anestad, Amelia Sieg, Olivia Judah

## File Listing
```
elevator/
├── Makefile
├── part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
│   └── Makefile
├── part2/
│   ├── src/
│   └── Makefile
├── part3/
│   ├── src/
│   ├── tests/
│   ├── Makefile
│   └── sys_call.c
├── Makefile
└── README.md

```
# How to Compile & Execute

### Requirements
- **Compiler**: `gcc` for C/C++

## Part 1

### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in ...
### Execution
```bash
make run
```
This will run the program ...

## Part 2

### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in ...
### Execution
```bash
make run
```
This will run the program ...


## Part 3

### Compilation
make clean
make

This will build the executable in ...

### Execution
sudo insmod elevator.ko

This will run the program ...

watch -n 1 cat /proc/elevator
./producer [number of passengers]
./consumer --start
./consumer --stop
sudo rmmod elevator.ko

## Bugs
- **Bug 1**: This is bug 1.
- **Bug 2**: This is bug 2.
- **Bug 3**: This is bug 3.

## Considerations
[Description]
