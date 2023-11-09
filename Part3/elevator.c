/*
WE ARE READY TO TRY AND COMPILE AND RUN!! 
- we should step though our logic before though

Michael questions:    
- How are producer.c and consumer.c connected/linked to our elevator.c?
- Do we need kfree when we move a passenger from waiting to inside the elevator
- We have a lot of locking and unlocking mutexes. Do we have too many? 

Random thoughts:
elevator active is the main funciton where you are moving up and down (where we call move algorithm)
stop would be checking for no flags?

Linked List API: html;//www.kernel.org/doc/html/v4.14/core-api/kernel-api.html

What we changed:
struct Passenger passengers[MAX_PASSENGERS] -> struct Passenger passengers[4]
struct Passenger passengersWaiting[NUM_FLOORS] -> struct Passenger passengersWaiting[5]
in moveLogic, int current -> int currentLoc (I think "current" is a built in thing, we can't use that word) - also changed it in process_elevator_state
list_rotate_left(&building.passengersWaiting[e_thread->current_floor-1].list); ADDED THE &
added & and .list to lines 300 and 393

*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/time.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("kernel module for elevator");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

// Global Variables
#define NUM_FLOORS 6
#define MAX_PASSENGERS 5 //this is max passengers inside elvator
#define MAX_WEIGHT 750
// need to lock when you write to proc file and need to brag information about the elevator
DEFINE_MUTEX(elevator_mutex); //mutex to control shared data access between floor and elevators
// mutex for floors becuase we needd to lock when we access the waiting passengers
// use this when you create new passengers waiting and when you remove passengers waiting 
    // and anytime you chage/access the waiting passengers
DEFINE_MUTEX(floor_mutex); 
static struct Elevator elevator_thread;
static struct Building building;
static int flag = 0; //0 for off, 1 for on
static int stop = 0; //0 for not trying to stop, 1 for trying to stop (change back to 0 after stopped)

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

enum state {OFFLINE, IDLE, LOADING, UP, DOWN}; // enums are just integers

static struct proc_dir_entry *proc_entry;

struct Passenger{ //struct to store all of the varibles that passenger needs
    int weight; 
    int start_floor;
    int destination_floor;  //this means passenger request
    int type; //initialized to -1 to indicate not a real passenger yet
    struct list_head list; //connect to next passenger
};

struct Elevator{
    int passengersServiced;
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


int moveLogic(struct Elevator *e_thread){ // this return how many floors you need to move up/down (not jsut 1/-1)
    /*
    SSTF - Shortest Seek Time First, prioritize going up if there is a tie (choose higher floor)
    Pick up as many people as we can up to 5 or up to 750lbs - isn't this addPassenger()?
    Scan all of their destination floors and grab the closest one first, continue
    At each floor, if any of the 5 passengers can get off, get off.
    Start at Floor 1
    */
    mutex_lock(&elevator_mutex);
    int currentLoc = e_thread->current_floor;
    mutex_unlock(&elevator_mutex);
    int closest = NUM_FLOORS+1; // how far away is the closest floor to drop someone off
    for(int i = 0; i < MAX_PASSENGERS; i++){
        mutex_lock(&elevator_mutex);
        int temp = (e_thread->passengers[i].type == -1);
        mutex_unlock(&elevator_mutex);
        if(temp){
            continue;
        }
        else{
            mutex_lock(&elevator_mutex);
            int distance = e_thread->passengers[i].destination_floor - currentLoc; //get distance this person needs to travel
            mutex_unlock(&elevator_mutex);
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
        mutex_unlock(&elevator_mutex);
        return 1;
    }

    // if flag is 0
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
    
    // kmalloc because making a new passenger and need to know we have memory to do so
    struct Passenger *new_passenger = kmalloc(sizeof(struct Passenger), GFP_KERNEL); 
    
    if(!new_passenger)
    {
        printk(KERN_INFO "Error: could not allocate memory for new passenger\n");
        mutex_unlock(&floor_mutex);
        return -ENOMEM;
    }

    // long ret = issue_request(start, dest, type);
    // printf("Issue (%d, %d, %d) returned %ld\n", start, dest, type, ret);
    new_passenger->type = type;
    new_passenger->weight = 100 + (type*50);
    new_passenger->start_floor = start_floor;
    new_passenger->destination_floor = destination_floor;

    building.num_people += 1;
    
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

    //not stopping already
    if(stop == 0){
        stop = 1; // indicate stop picking up new passengers
        return 0;
    }
    //already in the process of stopping
    return 1;
}

// this adds passenger to elevator not creating new passenger
void addPassenger(struct Elevator *e_thread){ //issue request calls this
    // The people that get off on this floor have already gotten off at this point 
    // Only call this function if there is someone waiting
    // This will only not add a passenger if the passenger would make the elevator overweight
    // This function adds as many passengers as it can (until 5 or 750lbs)
    // The passed in passenger is the first one waiting on this floor (it points to the next waiting one)
       //Passenger pointer to name kmalloc

    // need e lock
    mutex_lock(&elevator_mutex);
    int temp = (e_thread->numPassengers < MAX_PASSENGERS); // we don't have max
    mutex_unlock(&elevator_mutex);
    while(!(temp)){ // can fit another passenger - this shouldn't be ! 
        mutex_lock(&floor_mutex);
        mutex_lock(&elevator_mutex);
        int temp = (list_empty(&building.passengersWaiting[(e_thread->current_floor)-1].list) != 0);
        mutex_unlock(&elevator_mutex);
        mutex_unlock(&floor_mutex);

        if(temp) 
        {
            break; // leave while loop - no one waiting on floor
        }

        // Remove a waiting passenger and increase elevator->numPassengers - using kthread?
        mutex_lock(&elevator_mutex);
        mutex_lock(&floor_mutex);
        int temp = (e_thread->weight + building.passengersWaiting[e_thread->current_floor-1].weight <= MAX_WEIGHT);
        mutex_unlock(&floor_mutex);
        mutex_unlock(&elevator_mutex);
        if(temp){ //weight is okay
            for(int i = 0; i < MAX_PASSENGERS; i++){
                mutex_lock(&elevator_mutex);
                mutex_lock(&floor_mutex);
                if(e_thread->passengers[i].type == -1){
                    e_thread->passengers[i].type = building.passengersWaiting[e_thread->current_floor-1].type; 
                    e_thread->passengers[i].destination_floor = building.passengersWaiting[e_thread->current_floor-1].destination_floor;
                    e_thread->passengers[i].start_floor = building.passengersWaiting[e_thread->current_floor-1].start_floor;
                    e_thread->passengers[i].weight = building.passengersWaiting[e_thread->current_floor-1].weight;
                    e_thread->numPassengers++; // increase number of passengers
                    mutex_unlock(&elevator_mutex);
                    mutex_unlock(&floor_mutex);
                    break; // leave the for loop
                }
                mutex_unlock(&elevator_mutex);
                mutex_unlock(&floor_mutex);
            }
            //change the array that holds the waiting people
            mutex_lock(&elevator_mutex);
            mutex_lock(&floor_mutex);
            list_rotate_left(&building.passengersWaiting[e_thread->current_floor-1].list);
            building.num_people--; 
            mutex_unlock(&floor_mutex);
            mutex_unlock(&elevator_mutex);
        }
        else
            break; // next person is too heavy so break out of while loop 
    }

}         
        
void removePassenger(struct Elevator *e_thread) //need to pass in info to know which passenger to remove
{
    //remove passenger (they're exiting the elevator) based on their specified destination floor
    for(int i = 0; i < MAX_PASSENGERS; i++)
    {
        mutex_lock(&elevator_mutex);
        int temp = (e_thread->passengers[i].destination_floor == e_thread->current_floor);
        mutex_unlock(&elevator_mutex);
        if(temp) // same thing as if (e_thread->passengers[i].destination_floor == e_thread->current_floor){
        {
            ssleep(1); // NEVER have a lock while sleeping
            mutex_lock(&elevator_mutex);
            e_thread->weight -= e_thread->passengers[i].weight;
            e_thread->passengers[i].type = -1; 
            e_thread->numPassengers--;
            e_thread->passengersServiced++;
            mutex_unlock(&elevator_mutex);
        }
    }
    // You need to lock and unlock for small atomic sections (sections that need to finish before you can access the memory)
    
    // Can do: 
    //lock
    //if(){
        // unlock
    //}
    //else{
        //unlock
    //}

    //or you can do what he did above
}


int move_to_next_floor(struct Elevator *e_thread, int num){
    // -1 for down
    // 1 for up
    int newFloor = e_thread->current_floor + num;
    if((newFloor <= 0) || (newFloor > NUM_FLOORS))
        return 0;

    //else
    e_thread->current_floor = newFloor;
    return 1;
}

void process_elevator_state(struct Elevator *e_thread) { // Elevator waits 2.0 seconds when moving between floors and 1.0 seconds when loading/unloading passengers
    // We want to lock both mutex's up here and unlock after the switch statement 

    mutex_lock(&elevator_mutex);
    int temp = e_thread->status;
    mutex_unlock(&elevator_mutex);
    
    switch(temp) 
    {
        case UP:
            ssleep(2); // sleeps for 1 second, before processing next stuff!
            mutex_lock(&elevator_mutex);
            move_to_next_floor(e_thread, 1); // only move if not at floor 6
            e_thread->status = LOADING;
            mutex_unlock(&elevator_mutex);
            break;
        case DOWN:
            ssleep(2); // sleeps for 2 seconds, before processing next stuff!
            mutex_lock(&elevator_mutex);
            if(e_thread->current_floor != 1){
                e_thread->current_floor -= 1;
                // move_to_next_floor(e_thread, -1);
            }
            e_thread->status = LOADING;
            mutex_unlock(&elevator_mutex);
            break;
        case IDLE: // elevator is started with this and is also when there are no more passengers waiting
            // check if there are passengers waiting and if so then make it not idle anymore
            mutex_lock(&floor_mutex);
            int temp = (building.num_people > 0);
            mutex_unlock(&floor_mutex); 
            if(temp)
            {
                mutex_lock(&elevator_mutex);
                e_thread->status = LOADING; // loading will change it to up/down to get the people
                mutex_unlock(&elevator_mutex);
            }
            break;
        case OFFLINE:
            break;
        case LOADING:
            // unload and then load passengers

            removePassenger(e_thread); //this removes (unloads) and does ssleep(1)

            // load the  passengers
            if(stop == 0) // Not in the process of trying to stop
                addPassenger(e_thread);

            int dir = moveLogic(e_thread);
            
            mutex_lock(&elevator_mutex);
            int temp2 = (e_thread->numPassengers == 0);
            mutex_unlock(&elevator_mutex);
        
            if(temp2) // if nobody in the elevator
            {
                // find waiting passengers on other floors becuase no one in elevator and no one on this floor
                mutex_lock(&elevator_mutex);
                int direction = NUM_FLOORS+1; //direction to go - want it to be a number higher than the furthest distence someone needs to travel
                //so that line 379 is true and direction is changed
                int currentLoc = e_thread->current_floor;
                mutex_unlock(&elevator_mutex);

                for(int i = 0; i < NUM_FLOORS; i++)
                {
                    mutex_lock(&floor_mutex);
                    // if waiting passengers head != NULL
                    if(list_empty(&building.passengersWaiting[i].list) == 0) // not empty return 0 - if people waiting on floor
                    {
                        int distance = building.passengersWaiting[i].start_floor - currentLoc; //distance the elevator must travel to get to them
                        if(abs(distance) < abs(direction)) // abs so if need to go down 3 then distance -3
                        {
                            direction = distance; //make this -3 which is less than 0 - gets shortest distance we wanna travel
                        }
                    }
                    mutex_unlock(&floor_mutex);
                }

                if(direction == NUM_FLOORS+1){ // there was no one waiting (direction did not change and will always change if someone is waiting)
                    if(stop == 1) // emptied elevator and can stop it because stop_elevator() was called earlier 
                    // reminder that if stop = 1 that means you are in the PROCESS of trying to stop
                    {
                        mutex_lock(&elevator_mutex);
                        e_thread->status = OFFLINE; // OFFILINE means it sits on a floor until start_elevator is called
                        flag = 0; // flag shows that elevator is stopped
                        stop = 0; // no longer trying to stop, it is stopped
                        mutex_unlock(&elevator_mutex);
                    }
                    else // elevator is running but no one waiting on floors 
                    {
                        mutex_lock(&elevator_mutex);
                        e_thread->status = IDLE;
                        mutex_unlock(&elevator_mutex);
                    }
                }
                else if(direction < 0) // people waiting on floor below
                {
                    mutex_lock(&elevator_mutex);
                    e_thread->status = DOWN;
                    mutex_unlock(&elevator_mutex);
                }
                else
                {
                    mutex_lock(&elevator_mutex);
                    e_thread->status = UP; // people waiting on floor above
                    mutex_unlock(&elevator_mutex);
                }
                
                
            }
            // if  need to go up:
            else if(dir > 0)
            {
                mutex_lock(&elevator_mutex);
                e_thread->status = UP; // start going up to pickup passengers
                mutex_unlock(&elevator_mutex);
            } 
            else{
                // if need to go down:
                mutex_lock(&elevator_mutex);
                e_thread->status = DOWN; // start going  down to pickup passengers
                mutex_unlock(&elevator_mutex);
            }
            break;
        default:
            break;
    }
}

// function to print to proc file the bar state using waiter state
int print_building_state(char *buf, struct Elevator *e_thread) {
    int len = 0;

    // convert enums (integers) to strings
    const char *status[5] = {"OFFLINE", "IDLE", "LOADING", "UP", "DOWN"};

    mutex_lock(&elevator_mutex);
    len = sprintf(buf, "Elevator state: %s\n", status[e_thread->status]);
    len += sprintf(buf + len, "Current floor: %d\n", e_thread->current_floor);
    len += sprintf(buf + len, "Current weight: %d\n", e_thread->weight);
    len += sprintf(buf + len, "Elevator status:"); // This is who is inside the elevator
    mutex_unlock(&elevator_mutex);
    for(int i = 0; i < MAX_PASSENGERS; i++){
        mutex_lock(&elevator_mutex);
        switch (e_thread->passengers[i].type)
        {
        case -1:
            break;
        case 0:
            len += sprintf(buf + len, " F%d", e_thread->passengers[i].destination_floor);
            break;
        case 1:
            len += sprintf(buf + len, " O%d", e_thread->passengers[i].destination_floor);
            break;
        case 2:
            len += sprintf(buf + len, " J%d", e_thread->passengers[i].destination_floor);
            break;
        case 3:
            len += sprintf(buf + len, " S%d", e_thread->passengers[i].destination_floor);
            break;
        default:
            break;
        }
        mutex_unlock(&elevator_mutex);
    }
    len += sprintf(buf + len, "\n\n");

    for(int i = NUM_FLOORS-1; i >= 0; i--) {
        int floor = i+1;
        
        mutex_lock(&elevator_mutex);
        int temp = (floor != e_thread->current_floor);
        mutex_unlock(&elevator_mutex);
        if(temp)
        {
            len += sprintf(buf + len, "[ ] Floor %d:", floor);
        }
        else
        {
            len += sprintf(buf + len, "[*] Floor %d:", floor);
        }

        // print the people waiting
        // struct Passenger variable = building.passengersWaiting[i]; // IF PROC IS NOT WORKING LOOK HERE FIRST!!!!!!!!!!!!!!!!!!!!!!! IF PROC IS NOT WORKING LOOK HERE FIRST
        // MIGHT NEED KMALLOC HERE **************************************************************************************************************************************************************
        struct Passenger *variable;
        mutex_lock(&floor_mutex);
        int temp3 = (list_empty(&building.passengersWaiting[i].list) == 0);
        mutex_unlock(&floor_mutex);
        if(temp3)
        { // if not empty
            int total = 0;
            mutex_lock(&floor_mutex);
            list_for_each_entry(variable, &building.passengersWaiting[i].list, list){
                total++;
            }
            mutex_unlock(&floor_mutex);
            len += sprintf(buf + len, " %d", total);

            mutex_unlock(&elevator_mutex);
            mutex_lock(&floor_mutex);
            list_for_each_entry(variable, &building.passengersWaiting[i].list, list){ // go thru list of passengers waiting on floor i
                switch (variable->type)
                {
                    case -1:
                        break;
                    case 0:
                        len += sprintf(buf + len, " F%d", variable->destination_floor);
                        break;
                    case 1:
                        len += sprintf(buf + len, " O%d", variable->destination_floor);
                        break;
                    case 2:
                        len += sprintf(buf + len, " J%d", variable->destination_floor);
                        break;
                    case 3:
                        len += sprintf(buf + len, " S%d", variable->destination_floor);
                        break;
                    default:
                        break;
                }
            }
            mutex_unlock(&floor_mutex);
            mutex_unlock(&elevator_mutex);
        }
        else // no one waiting on floor i
        {
            len += sprintf(buf + len, " 0");
        }
        len += sprintf(buf + len, "\n");
    }

    len += sprintf(buf + len, "\n");


    mutex_lock(&elevator_mutex);
    len += sprintf(buf + len, "Number of passengers: %d\n", e_thread->numPassengers);
    mutex_unlock(&elevator_mutex);

    mutex_lock(&floor_mutex);
    len += sprintf(buf + len, "Number of passengers waiting: %d\n", building.num_people);
    // len += sprintf(buf + len, "Number of passengers serviced: %d\n", e_thread->passengersServiced);
    mutex_unlock(&floor_mutex);

    return len;
}


static ssize_t procfile_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos){ // COME BACK TO ME PLZ
    
    char buf[256];
    int len = 0;

    if (*ppos > 0 || count < 256)
        return 0;


    // recall that this is triggered every second if we do watch -n1 cat /proc/waiter
    len = print_building_state(buf, &elevator_thread);   // this is how we write to the file!

    return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user

}

/* This is where we define our procfile operations */
static struct proc_ops procfile_pops = {
	.proc_read = procfile_read,
};

int elevator_active(void *_elevator) {
    struct Elevator *e_thread = (struct Elevator *) _elevator;

    while(!kthread_should_stop()) {
        process_elevator_state(e_thread);
    }

    return 0;
}

int spawn_elevator(struct Elevator *e_thread) {

    e_thread->kthread =
        kthread_run(elevator_active, e_thread, "thread elevator\n"); // thread actually spawns here!

    return 0;
}


/* This is where your kernel module will
   start from, as it gets loaded. */
static int __init elevator_init(void) { 
    // This is where we link our system calls to our stubs
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;

    for(int i = 0; i < NUM_FLOORS; i++){
        mutex_lock(&floor_mutex);
        INIT_LIST_HEAD(&building.passengersWaiting[i].list);
        mutex_unlock(&floor_mutex);
    }



    spawn_elevator(&elevator_thread); // this is where the elevator moves and does everything
    
    mutex_lock(&elevator_mutex);
    static int current_floor = 3; 
    elevator_thread.current_floor = current_floor;
    elevator_thread.status = OFFLINE; 
    elevator_thread.numPassengers = 0; 
    elevator_thread.passengersServiced = 0;
    mutex_unlock(&elevator_mutex);


    for(int i = 0; i < MAX_PASSENGERS; i++) // This for initializing the passengers INSIDE elevator
    {
        mutex_lock(&elevator_mutex);
        elevator_thread.passengers[i].type = -1;
        mutex_unlock(&elevator_mutex);
    }

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

    return 0; // only ever returns 0 (is this correct?) 

}

//free, unhook threads and sys calls, stops
static void __exit elevator_exit(void)
{
    struct Passenger *passenger, *next;

    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;


    for(int i = 0; i < NUM_FLOORS; i++){
        mutex_lock(&floor_mutex);
        list_for_each_entry_safe(passenger, next, &building.passengersWaiting[i].list, list) {
            list_del(&passenger->list);
            kfree(passenger);
        }
        mutex_unlock(&floor_mutex);
    }

    

    // Free the memory we allocated in init
    for(int i = 0; i < MAX_PASSENGERS; i++){
        mutex_lock(&elevator_mutex);
        kfree(&elevator_thread.passengers[i]);
        mutex_unlock(&elevator_mutex);
    }

    
    kthread_stop(elevator_thread.kthread); // this is where we stop the thread.

    remove_proc_entry(ENTRY_NAME, PARENT);

}

module_init(elevator_init);
module_exit(elevator_exit);
