/*
Random thoughts:
How do we know when we are fully fully done
If there is no one on elevator but people waiting, how do we know where to go
When you remove a person from the elevator array you need to set that slot as  NULL
Why is issue request being called in waiting passengers


*/
/*
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("kernel module for elevator");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

// global variables
static const int NUM_FLOORS = 6;
static const int MAX_PASSENGERS = 5;
static const int MAX_WEIGHT = 750;
static const int num_of_original_passengers = 10;
DEFINE_MUTEX(elevator_mutex); //mutex to control shared data access between floor and elevators
static struct Elevator elevator_thread;
static struct Building building;
static int flag = 0; //0 for off, 1 for on

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

enum state {OFFLINE, IDLE, LOADING, UP, DOWN}; // enums are just integers

struct Passenger{ //struct to store all of the varibles that passenger needs
    int weight; 
    int start_floor;
    int destination_floor;  //this means passenger request
    int type; //initialized to -1 to indicate not a real passenger yet
    struct list_head next_passenger; //connect to next passenger
};

struct Elevator{
    enum state status;
    int current_floor;
    int numPassengers; //this is just a count of how many in elevator
    int weight;
    struct task_struct *kthread; //struct to make a kthread to control elevator movement
    struct Passenger passengers[MAX_PASSENGERS]; //array of 5 passengers for inside the elevator
};

struct Building{
    int num_people; // how many people originally added in
    struct Passenger passengersWaiting[NUM_FLOORS]; //each section of array holds the first passenger on each floor
};

static struct proc_dir_entry *proc_entry;

int start_elevator(void){
    flag = 1;
    // The start_elevator() system call activates the elevator for service. 
    // From this point forward, the elevator exists and will begin to service requests. 
    // It returns 1 if the elevator is already active, 0 for a successful start, and -ERRORNUM 
    // If initialization fails or -ENOMEM if it couldn't allocate memory. 
    // The elevator is initialized with the following values:
    // State: IDLE
    // Current floor: 1
    // Number of passengers: 0 

    return 0;
}

int issue_request(int start_floor, int destination_floor, int type){
    //The issue_request() system call creates a request for a passenger, 
    //specifying the start floor, destination floor, and type of passenger 
    //(0 for freshmen, 1 for sophomore, 2 for junior, 3 for senior). 
    //It returns 1 if the request is invalid (e.g., out of range or invalid type) and 0 otherwise
    // call addPassengers()
    // call removePassengers()
    // call move_to_next_floor()
    return 0;
}

int stop_elevator(void){
    //The stop_elevator() system call deactivates the elevator. 
    //It stops processing new requests (passengers waiting on floors), 
    //but it must offload all current passengers before complete deactivation. 
    //Only when the elevator is empty can it be deactivated (state = OFFLINE). 
    //The system call returns 1 if the elevator is already in the process of deactivating and 0 otherwise. 
    flag = 0;
    return 0;
}

void addPassenger(struct Elevator *elevator, struct Passenger *passenger, struct Building *b){ //issue request calls this
    // The people that get off on this floor have already gotten off at this point 
    // Only call this function if there is someone waiting
    // This will only not add a passenger if the passenger would make the elevator overweight
    // This function adds as many passengers as it can (until 5 or 750lbs)
    // The passed in passenger is the first one waiting on this floor (it points to the next waiting one)
   
    if(people_on_floor){
        while(!elevator_full){ // can fit another passenger
            if(passenger == NULL)
                break; // leave while loop - no one waiting on floor
            // Remove a waiting passenger and increase elevator->numPassengers - using kthread?
                if(elevator->weight + passenger->weight <= MAX_WEIGHT){ //weight is okay
                    // add the person to elevator array
                    // go through elevator array (passengers)
                    // if spot is available take it
                    for(int i = 0; i < MAX_PASSENGERS; i++){
                        if(elevator->passengers[i] == NULL){
                            elevator->passengers[i] = passenger; // Maybe kmalloc goes here ************************************
                            elevator->numPassengers++; // increase number of passengers
                            break; // leave the for loop
                        }
                    }
                    //change the array that holds the waiting people
                    b->passengersWaiting[elevator->current_floor-1] = passenger->next_passenger;
                }
            } 
        }

    //Passenger pointer to name kmalloc
    struct Passenger * new_passenger = kmalloc(sizeof(struct Passenger), GFP_KERNEL);
}         
        
void removePassenger(struct Elevator *elevator) //need to pass in info to know which passenger to remove
{
    //remove passenger (they're exiting the elevator) based on their specified destination floor
    for(int i = 0; i < MAX_PASSENGERS; i++)
    {
        if(elevator->passengers[i].destination_floor == elevator->current_floor)
        {
            elevator->weight -= elevator->passengers[i].weight;
            elevator->passengers[i] = NULL; // kmalloc ************************************************************************
            elevator->numPassengers--;
        }
    }
}

bool elevator_full(struct Elevator *elevator){ // function to check if elevator is at full capacity, returns true if full
    return elevator->numPassengers >= MAX_PASSENGERS;
}

bool people_on_floor(struct Building *b, struct Elevator *elevator){ //check if floor has people on it to know if we should stop, returns true if there are
    return b->passengersWaiting[elevator->current_floor] != NULL; // kmalloc??????******************************************************************************************
}

int move_to_next_floor(struct Elevator *elevator, struct Passenger *passenger){
    /*
    SSTF - Shortest Seek Time First, prioritize going up if there is a tie (choose higher floor)
    Pick up as many people as we can up to 5 or up to 750lbs - isn't this addPassenger()?
    Scan all of their destination floors and grab the closest one first, continue
    At each floor, if any of the 5 passengers can get off, get off.
    Start at Floor 1
    */

    // for(int i = 0; i < passengersWaiting->destination_floor; i++)
    for(int i = 0; i < elevator->passengers[i]; i++){ // go through all the waiting passengers at floor (start at floor 1)
        int distance = (elevator->current_floor) - (passenger->destination_floor); // calculate the distances needed to be traveled for each of the waiting passengers
        // compare all the distances needed to be traveled, take the shortest one and go there, drop them off with removePassenger()
        // (elevator->current_floor) + distance
   }

   return (elevator->current_floor + 1) % NUM_FLOORS;

}

void process_elevator_state(struct Elevator *e_thread) {
    switch(e_thread->status) {
        case UP:
            ssleep(1); // sleeps for 1 second, before processing next stuff!
            e_thread->current_floor = move_to_next_floor(e_thread->current_floor);
            e_thread->status = UP;
            break;
        case DOWN:
            ssleep(2); // sleeps for 2 seconds, before processing next stuff!
            e_thread->status = DOWN; // changed states!
        // case IDLE
        // OFFLINE 
        // LOADING 
        default:
            break;
    }
}

int elevator_active(void *_elevator) {
    struct Elevator *e_thread = (struct Elevator *) _elevator;
    printk(KERN_INFO "Elevator thread has started running \n");
    while(!kthread_should_stop()) {
        process_elevator_state(e_thread);
    }
    return 0;
}

int spawn_elevator(struct Elevator *e_thread) {
    static int current_floor = 0;

    e_thread->current_floor = current_floor;
    e_thread->kthread =
        kthread_run(elevator_active, e_thread, "thread elevator\n"); // thread actually spawns here!

    return 0;
}

// function to print to proc file the bar state using waiter state
int print_building_state(char *buf) {
    int i;
    int len = 0;

    // convert enums (integers) to strings
    const char *status[5] = {"OFFLINE", "IDLE", "LOADING", "UP", "DOWN"};

    len += sprintf(buf + len, "Elevator state: %s\n", status[elevator_thread.status]);
    for(i = 0; i < NUM_FLOORS; i++) {
        int floor = i + 1;

        // ternary operators equivalent to the bottom if statement.
        // len += (i != elevator_thread.current_floor)
        //     ? sprintf(buf + len, "[ ] Floor %d: %d times serviced. \n", floor, bar.tables[i])
        //     : sprintf(buf + len, "[*] Floor %d: %d times serviced. \n", floor, bar.tables[i]);

        // if(i != elevator_thread.current_floor)
        //     len += sprintf(buf + len, "[ ] Floor %d: %d times cleaned. \n", i, bar.tables[i]);
        // else
        //     len += sprintf(buf + len, "[*] Floor %d: %d times cleaned. \n", i, bar.tables[i]);
    }
    return len;
}

//function to create random passengers waiting on random floors of the building
struct Passenger createWaitingPassenger(void)
{
    int type;
	int start;
	int dest;
	int num;
	srand(time(0)); //for random generation

	//sscanf(argv[1],"%d",&num);
    type = rnd(0,3); //getting grade letter

    start = rnd(1, 6); //getting floor
    do {
        dest = rnd(1, 6);
    } while(dest == start); //destination floor can not be start floor

    long ret = issue_request(start, dest, type);
    printf("Issue (%d, %d, %d) returned %ld\n", start, dest, type, ret);
    struct Passenger created;
    created.type = type;
    created.weight = 100 + (type*50);
    created.start_floor = start;
    created.destination_floor = dest;
    created.next_passenger = NULL; //kmalloc ???**********************************************************************************
    return created;
}

static ssize_t procfile_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos){ // COME BACK TO ME PLZ
    //syscalls_log.c    
    return simple_read_from_buffer(ubuf, count, ppos, log_buffer, buf_offset);

    //from waiter.c
    char buf[256];
    int len = 0;

    if (*ppos > 0 || count < 256)
        return 0;

    // recall that this is triggered every second if we do watch -n1 cat /proc/waiter
    len = print_bar_state(buf);   // this is how we write to the file!

    return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user

}

/* This is where we define our procfile operations */
static struct proc_ops procfile_pops = {
	.proc_read = procfile_read,
};

/* Treat this like a main function, this is where your kernel module will
   start from, as it gets loaded. */
static int __init elevator_init(void) {

    spawn_elevator(&elevator_thread); 

    if(IS_ERR(elevator_thread.kthread)) {
        printk(KERN_WARNING "error creating thread");
        remove_proc_entry(ENTRY_NAME, PARENT);
        return PTR_ERR(elevator_thread.kthread);
    }

    // This is where we link our system calls to our stubs
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;

    proc_entry = proc_create( // this is where we create the proc file!
        ENTRY_NAME,
        PERMS,
        PARENT,
        &procfile_pops
    );

    return 0;

}

//free, unhook threads and sys calls, stops
static void __exit elevator_exit(void)
{
    kthread_stop(elevator_thread.kthread); // this is where we stop the thread.

    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;

    remove_proc_entry(ENTRY_NAME, PARENT);
}

module_init(elevator_init);
module_exit(elevator_exit);