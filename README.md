# Dorm Elevator

[Description]

## Group Members
- **Amelia Sieg**: ats20b@fsu.edu
- **Olivia Judah**: ogj21@fsu.edu
- **Rebecca Anestad**: rba20@fsu.edu

## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: [Description]
- **Assigned to**: Amelia Sieg, Olivia Judah

### Part 2: Timer Kernel Module
- **Responsibilities**: [Description]
- **Assigned to**: Rebecca Anestad, Amelia Sieg

### Part 3a: Adding System Calls
- **Responsibilities**: [Description]
- **Assigned to**: Olivia Judah, Amelia Sieg

### Part 3b: Kernel Compilation
- **Responsibilities**: [Description]
- **Assigned to**: Amelia Sieg, Olivia Judah, Rebecca Anestad

### Part 3c: Threads
- **Responsibilities**: [Description]
- **Assigned to**: Olivia Judah, Rebecca Anestad

### Part 3d: Linked List
- **Responsibilities**: [Description]
- **Assigned to**: Rebecca Anestad, Amelia Sieg

### Part 3e: Mutexes
- **Responsibilities**: [Description]
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
