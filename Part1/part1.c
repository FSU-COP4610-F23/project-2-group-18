#include <stdio.h>
#include <unistd.h>

int main() {
    // System Call 1: Print a message
    write(1, "This is a system call test.\n", 27);
    
    // System Call 2: Sleep for 1 second
    sleep(1);
    
    // System Call 3: Create a new process
    fork();
    
    // System Call 4: Exit the program
    _exit(0);
}