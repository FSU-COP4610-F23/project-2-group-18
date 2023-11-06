/*
Michael questions:
- Can more people be spawned in after the initial passengers? 
    If so, how? (does the user call issue_request somehow?)
    YES - when you run ./producer.c #, you create # more waiting passengers 
- Where is start_elevator called? 
    In init
- How do we add kmalloc to the addPassenger function?
    Yes but we don't have it in the right spot
    
- How are producer.c and consumer.c connected/linked to our elevator.c?
- What things have their own threads (is the elevator its own thread and producer and consumer and building?)


Random thoughts:
How do we know when we are fully fully done
If there is no one on elevator but people waiting, how do we know where to go
When you remove a person from the elevator array you need to set that slot's type as -1
Why is issue request being called in waiting passengers
Where to add the logic to know what floor to move to next
To print linked list use list_for_each

-things should be offline before unloading ??

if it is offline, it is going to wait for a flag to start it
start will have the flag - this is where we implement the mutex
issue will add the passengers 
elevator active is the main funciton where you are moving up and down (where we call move algorithm)
stop would be checking for no flags?


Linked List API: html;//www.kernel.org/doc/html/v4.14/core-api/kernel-api.html


**need to initialize the array of passengers in elevator and put passengers in with type == -1 when elevator created (not started)

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

// Global Variables
static const int NUM_FLOORS = 6;
static const int MAX_PASSENGERS = 5; //this is max passengers inside elvator
static const int MAX_WEIGHT = 750;
static const int num_of_original_passengers = 10;
// need to lock when you write to proc file and need to brag information about the elevator
DEFINE_MUTEX(elevator_mutex); //mutex to control shared data access between floor and elevators
// mutex for floors becuase we needd to lock when we access the waiting passengers
// use this when you create new passengers waiting and when you remove passengers waiting 
    // and anytime you chage/access the waiting passengers
DEFINE_MUTEX(floor_mutex); 
static struct Elevator elevator_thread;
static struct Building building;
static int flag = 0; //0 for off, 1 for on
static int passengersServiced = 0;

#define BUFFER_SIZE 5 // We may not use this
static int buffer[BUFFER_SIZE];
static int count = 0;

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

enum state {OFFLINE, IDLE, LOADING, UP, DOWN}; // enums are just integers

struct Passenger{ //struct to store all of the varibles that passenger needs
    int weight; 
    int start_floor;
    int destination_floor;  //this means passenger request
    int type; //initialized to -1 to indicate not a real passenger yet
    struct list_head list; //connect to next passenger
};

struct Elevator{
    enum state status;
    int current_floor;
    int numPassengers; //this is just a count of how many in elevator
    int weight;
    struct task_struct *kthread; //struct to make a kthread to control elevator movement
    struct Passenger passengers[MAX_PASSENGERS]; //array of 5 passengers for inside the elevator
    //  for loop 
    // init list head

    // list add tail to issue request 
};

struct Building{
    int num_people; // how many people originally added in
    struct Passenger passengersWaiting[NUM_FLOORS]; //each section of array holds the first passenger on each floor
};

// struct Passenger dummy;
// dummy->type = -1;

static struct proc_dir_entry *proc_entry;

int moveLogic(struct Elevator *e_thread){ // this return how many floors you need to move up/down (not jsut 1/-1)
    /*
    SSTF - Shortest Seek Time First, prioritize going up if there is a tie (choose higher floor)
    Pick up as many people as we can up to 5 or up to 750lbs - isn't this addPassenger()?
    Scan all of their destination floors and grab the closest one first, continue
    At each floor, if any of the 5 passengers can get off, get off.
    Start at Floor 1
    */
    int current = e_thread->current_floor;
    int closest = NUM_FLOORS+1; // how far away is the closest floor to drop someone off
    for(int i = 0; i < MAX_PASSENGERS; i++){
        if(e_thread->passengers[i].type == -1){
            continue;
        }
        else{
            int distance = current - e_thread->passengers[i].destination_floor; //get distance this person needs to travel
            if(abs(distance) < abs(closest))
            {
                closest = distance;
            }
        }
    }
    return closest;
}



int start_elevator(void){
    //call process_elevator_state
    mutex_lock(&elevator_mutex);
    if(flag == 1)
    {
        // elevator is already active so return 1
        return 1;
    }

    flag = 1; // two mutexes one in proc read to lock so it doesn't get modified, elevator needs mutex, and floors 
    elevator_thread.status = IDLE; 
    mutex_unlock(&elevator_mutex);
    return 0;

    // The start_elevator() system call activates the elevator for service. 
    // From this point forward, the elevator exists and will begin to service requests. 
    // It returns 1 if the elevator is already active, 0 for a successful start, and -ERRORNUM 
    // If initialization fails or -ENOMEM if it couldn't allocate memory. 

}

// This should be done
int issue_request(int start_floor, int destination_floor, int type){

    //The issue_request() system call creates a request for a passenger, 
    //specifying the start floor, destination floor, and type of passenger 
    //(0 for freshmen, 1 for sophomore, 2 for junior, 3 for senior). 
    //It returns 1 if the request is invalid (e.g., out of range or invalid type) and 0 otherwise
    // call addPassengers(
    
    mutex_lock(&floor_mutex);
    struct Passenger *new_passenger = kmalloc(sizeof(struct Passenger), GFP_KERNEL);
    if(!new_passenger) {
        printk(KERN_INFO "Error: could not allocate memory for new passenger\n");
        return -ENOMEM;
    }

    // long ret = issue_request(start, dest, type);
    // printf("Issue (%d, %d, %d) returned %ld\n", start, dest, type, ret);
    new_passenger->type = type;
    new_passenger->weight = 100 + (type*50);
    new_passenger->start_floor = start_floor;
    new_passenger->destination_floor = destination_floor;

    building.num_people++;
    
    // add passenger with list_add_tail 
    list_add_tail(&new_passenger->list, &building.passengersWaiting[start_floor-1].list); // this is how we add to the list

    mutex_unlock(&floor_mutex);

    return 0;
}

int stop_elevator(void){
    //The stop_elevator() system call deactivates the elevator. 
    //It stops processing new requests (passengers waiting on floors), 
    //but it must offload all current passengers before complete deactivation. 
    //Only when the elevator is empty can it be deactivated (state = OFFLINE). 
    //The system call returns 1 if the elevator is already in the process of deactivating and 0 otherwise. 
    mutex_lock(&elevator_mutex);
    // remove all passengers in the elevator
    flag = 0;
    elevator_thread.status = OFFLINE; // This pauses the elevator but it stille exists
    mutex_unlock(&elevator_mutex);
    return 0;
}

// this adds passenger to elevator not creating new passenger
void addPassenger(struct Elevator *e_thread){ //issue request calls this
    // The people that get off on this floor have already gotten off at this point 
    // Only call this function if there is someone waiting
    // This will only not add a passenger if the passenger would make the elevator overweight
    // This function adds as many passengers as it can (until 5 or 750lbs)
    // The passed in passenger is the first one waiting on this floor (it points to the next waiting one)
       //Passenger pointer to name kmalloc


    mutex_lock(&floor_mutex);
    mutex_lock(&elevator_mutex);
    while(!elevator_full){ // can fit another passenger
        if(list_empty(building.passengersWaiting[e_thread->current_floor].type) != 0) // change it to be NULL with some funtion ********************************************
            break; // leave while loop - no one waiting on floor
        
        // Remove a waiting passenger and increase elevator->numPassengers - using kthread?
        if(e_thread->weight + building.passengersWaiting[e_thread->current_floor-1].weight <= MAX_WEIGHT){ //weight is okay
            // add the person to elevator array
            // go through elevator array (passengers)
            // if spot is available take it
            for(int i = 0; i < MAX_PASSENGERS; i++){
                if(e_thread->passengers[i].type == -1){
                    e_thread->passengers[i].type = building.passengersWaiting[e_thread->current_floor-1].type; 
                    e_thread->passengers[i].destination_floor = building.passengersWaiting[e_thread->current_floor-1].destination_floor;
                    e_thread->passengers[i].start_floor = building.passengersWaiting[e_thread->current_floor-1].start_floor;
                    e_thread->passengers[i].weight = building.passengersWaiting[e_thread->current_floor-1].weight;
                    e_thread->numPassengers++; // increase number of passengers
                    break; // leave the for loop
                }
            }
            //change the array that holds the waiting people
            list_rotate_left(building.passengersWaiting[e_thread->current_floor-1].list);
            building.num_people--; 
        }
        else
            break; // next person is too heavy so break out of while loop 
    }
    
    mutex_unlock(&elevator_mutex);
    mutex_unlock(&floor_mutex);

}         
        
void removePassenger(struct Elevator *e_thread) //need to pass in info to know which passenger to remove
{
    //remove passenger (they're exiting the elevator) based on their specified destination floor
    for(int i = 0; i < MAX_PASSENGERS; i++)
    {
        if(e_thread->passengers[i].destination_floor == e_thread->current_floor)
        {
            ssleep(1);
            e_thread->weight -= e_thread->passengers[i].weight;
            e_thread->passengers[i].type = -1; 
            e_thread->numPassengers--;
            passengersServiced++;
        }
    }
}

bool elevator_full(struct Elevator *e_thread){ // function to check if elevator is at full capacity, returns true if full
    if (e_thread->numPassengers >= MAX_PASSENGERS)
    {
        return true;
    }
    return false;
}


int move_to_next_floor(struct Elevator *e_thread, int num){


//     // for(int i = 0; i < passengersWaiting->destination_floor; i++)
//     for(int i = 0; i < elevator->passengers[i]; i++){ // go through all the waiting passengers at floor (start at floor 1)
//         int distance = (elevator->current_floor) - (passenger->destination_floor); // calculate the distances needed to be traveled for each of the waiting passengers
//         // compare all the distances needed to be traveled, take the shortest one and go there, drop them off with removePassenger()
//         // (elevator->current_floor) + distance
//    }

//    return (elevator->current_floor + 1) % NUM_FLOORS;


   // pass in int called num
   // -1 for down
   // 1 for up
   return e_thread->current_floor += num;

}

void process_elevator_state(struct Elevator *e_thread) { // Elevator waits 2.0 seconds when moving between floors and 1.0 seconds when loading/unloading passengers
    switch(e_thread->status) {
        case UP:
            ssleep(2); // sleeps for 1 second, before processing next stuff!
            mutex_lock(&elevator_mutex);
            e_thread->current_floor = move_to_next_floor(e_thread->current_floor, 1);
            if(e_thread->numPassengers == 0){ // no one in elevator so try and load passengers
                e_thread->status = LOADING;
            }
            else{
                for(int i = 0; i < MAX_PASSENGERS; i++){
                    if(e_thread->passengers[i].type != -1){
                        if(e_thread->passengers[i].destination_floor == e_thread->current_floor){
                            e_thread->status = LOADING;
                            break;
                        }
                    }
                }
            }
            mutex_unlock(&elevator_mutex);
            // if no passengers need to get off on this floor, the status is still UP which is good
            break;
        case DOWN:
            ssleep(2); // sleeps for 2 seconds, before processing next stuff!
            mutex_lock(&elevator_mutex);
            e_thread->current_floor = move_to_next_floor(e_thread->current_floor, -1);
            if(e_thread->numPassengers == 0){ // no one in elevator so try and load passengers
                e_thread->status = LOADING;
            }
            else{
                for(int i = 0; i < MAX_PASSENGERS; i++){
                    if(e_thread->passengers[i].type != -1){ // only check if there is a valid passenger in this spot
                        if(e_thread->passengers[i].destination_floor == e_thread->current_floor){
                            e_thread->status = LOADING;
                            break;
                        }
                    }
                }
            }
            mutex_unlock(&elevator_mutex);
            break;
        case IDLE: // elevator is initialized with this and is also when there are no more passengers waiting
            // check if there are passengers waiting and if so then make it not idle anymore
            mutex_lock(&floor_mutex);
           
            if(building.num_people > 0){
                for(int i = 0; i < NUM_FLOORS; i++)
                {
                    mutex_lock(&elevator_mutex);
                    e_thread->status = LOADING; // loading will change it to up/down to get the people
                    mutex_unlock(&elevator_mutex);
                }
            }
            
            mutex_unlock(&floor_mutex);
            
            break;
        case OFFLINE:
            break;
        case LOADING:
            // unload and then load passengers
            mutex_lock(&elevator_mutex);
            
            removePassenger(e_thread); //this removes (unloads) and does ssleep(1)
            // load the damn passengers
            addPassenger(e_thread);

            int dir = moveLogic(e_thread);
            if(e_thread->numPassengers == 0)
            {
                // find waiting passengers on other floors
                mutex_lock(&floor_mutex);
                
                int direction = NUM_FLOORS+1; //direction to go
                int current = e_thread->current_floor;
                
                for(int i = 0; i < NUM_FLOORS; i++)
                {
                    // if waiting passengers head != NULL
                    if(list_empty(building.passengersWaiting[i]) == 0) // not empty return 0
                    {
                        int distance = building.passengersWaiting[i].start_floor - current; //get distance this person needs to travel
                        if(abs(distance) < abs(direction))
                        {
                            direction = distance;
                        }
                    }
                }
                if(direction == NUM_FLOORS+1){
                    // there was noone waiting; omg; woah
                    e_thread->status = IDLE;
                }
                else if(direction < 0)
                    e_thread->status = DOWN;
                else
                    e_thread->status = UP;
                
                mutex_unlock(&floor_mutex);
            }
            // if  need to go up:
            else if(dir > 0)
            {
                e_thread->status = UP; // start going up 
            } 
            else{
                // if need to go down:
                e_thread->status = DOWN; // start going  down
            }
            mutex_unlock(&elevator_mutex);
            break;
        default:
            break;
    }
}

// function to print to proc file the bar state using waiter state
int print_building_state(char *buf) {
    int len = 0;

    // convert enums (integers) to strings
    const char *status[5] = {"OFFLINE", "IDLE", "LOADING", "UP", "DOWN"};

    mutex_lock(&elevator_mutex);

    len = sprintf(buf, "Elevator state: %s\n", status[elevator_thread.status]);
    len = sprintf(buf + len, "Current floor: %d\n", elevator_thread.current_floor);
    len = sprintf(buf + len, "Current weight: %d\n", elevator_thread.weight);
    len = sprintf(buf + len, "Elevator status: "); // This is who is inside the elevator
    for(int i = 0; i < MAX_PASSENGERS; i++){
        switch (elevator_thread.passengers[i].type)
        {
        case -1:
            break;
        case 0:
            len = sprintf(buf + len, "F%d\n", elevator_thread.passengers[i].destination_floor);
            break;
        case 1:
            len = sprintf(buf + len, "O%d\n", elevator_thread.passengers[i].destination_floor);
            break;
        case 2:
            len = sprintf(buf + len, "J%d\n", elevator_thread.passengers[i].destination_floor);
            break;
        case 3:
            len = sprintf(buf + len, "S%d\n", elevator_thread.passengers[i].destination_floor);
            break;
        default:
            break;
        }
        
    }
    
    mutex_lock(&floor_mutex);
    
    for(int i = 0; i < NUM_FLOORS; i++) {
        int floor = i+1;

        if(i != elevator_thread.current_floor)
            len += sprintf(buf + len, "[ ] Floor %d: ", floor);  //needs third
        else
            len += sprintf(buf + len, "[*] Floor %d:", floor);

        // print the people waiting
        struct Passenger variable = building.passengersWaiting[i]; // IF PROC IS NOT WORKING LOOK HERE FIRST!!!!!!!!!!!!!!!!!!!!!!! IF PROC IS NOT WORKING LOOK HERE FIRST
        if(list_empty(building.passengersWaiting[i]) == 0){ // if not empty
            int total = 0;
            list_for_each(variable, building.passengersWaiting[i].list){
                total++;
            }
            len = sprintf(buf + len, " %d", total);

            list_for_each(variable, building.passengersWaiting[i].list){ // go thru list of passengers waiting on floor i
                switch (variable.type)
                {
                    case -1:
                        break;
                    case 0:
                        len = sprintf(buf + len, " F%d", elevator_thread.passengers[i].destination_floor);
                        break;
                    case 1:
                        len = sprintf(buf + len, " O%d", elevator_thread.passengers[i].destination_floor);
                        break;
                    case 2:
                        len = sprintf(buf + len, " J%d", elevator_thread.passengers[i].destination_floor);
                        break;
                    case 3:
                        len = sprintf(buf + len, " S%d", elevator_thread.passengers[i].destination_floor);
                        break;
                    default:
                        break;
                }
            }
        }
        else // no one waiting on floor i
        {
            len = sprintf(buf + len, " 0");
        }
        len = sprintf(buf + len, "\n");
    }
    
    mutex_unlock(&floor_mutex);

    len = sprintf(buf + len, "Number of passengers: \n", elevator_thread.numPassengers);
    len = sprintf(buf + len, "Number of passengers waiting: \n", building.passengersWaiting);
    len = sprintf(buf + len, "Number of passengers serviced: \n", passengersServiced);

    mutex_unlock(&elevator_mutex);

    return len;
}


static ssize_t procfile_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos){ // COME BACK TO ME PLZ
    
    char buf[256];
    int len = 0;

    if (*ppos > 0 || count < 256)
        return 0;


    // recall that this is triggered every second if we do watch -n1 cat /proc/waiter
    len = print_building_state(buf);   // this is how we write to the file!

    return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user

}

/* This is where we define our procfile operations */
static struct proc_ops procfile_pops = {
	.proc_read = procfile_read,
};


int spawn_elevator(struct Elevator *e_thread) {

    e_thread->kthread =
        kthread_run(elevator_active, e_thread, "thread elevator\n"); // thread actually spawns here!

    return 0;
}

int elevator_active(void *_elevator) {
    struct Elevator *e_thread = (struct Elevator *) _elevator;
    // printk(KERN_INFO "Elevator thread has started running \n");
    while(!kthread_should_stop()) {
        process_elevator_state(e_thread);
    }
    return 0;
}

/* This is where your kernel module will
   start from, as it gets loaded. */
static int __init elevator_init(void) {
    // This is where we link our system calls to our stubs
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;

    mutex_lock(&floor_mutex);

    for(int i = 0; i < NUM_FLOORS; i++){
        INIT_LIST_HEAD(&building.passengersWaiting[i].list);
    }

    mutex_unlock(&floor_mutex);

    mutex_lock(&elevator_mutex);

    spawn_elevator(&elevator_thread); // this is where the elevator moves and does everything

    static int current_floor = 1; 
    elevator_thread.current_floor = current_floor; 
    elevator_thread.status = OFFLINE; 
    elevator_thread.numPassengers = 0; 

    mutex_unlock(&elevator_mutex);


    if(IS_ERR(elevator_thread.kthread)) {
        printk(KERN_WARNING "error creating thread");
        remove_proc_entry(ENTRY_NAME, PARENT);
        return PTR_ERR(elevator_thread.kthread);
    }

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
    struct Passenger *passenger, *next;
    kthread_stop(elevator_thread.kthread); // this is where we stop the thread.

    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;

    // need to kfree() anything that you do kmalloc to
    // // Clean up the linked list and free memory.
    list_for_each_entry_safe(passenger, next, &building.passengersWaiting[0].list, list) {
        list_del(&passenger->list);
        kfree(passenger);
    }

    remove_proc_entry(ENTRY_NAME, PARENT);

}

module_init(elevator_init);
module_exit(elevator_exit);