// Rebecca Anestad, Olivia Judah, Amelia Sieg
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
#define MAX_PASSENGERS 5 // max passengers inside elevator
#define MAX_WEIGHT 750

DEFINE_MUTEX(elevator_mutex); // mutex to control shared data access between floor and elevators
DEFINE_MUTEX(floor_mutex); 

static struct Elevator elevator_thread;
static struct Building building;
static int flag = 0; // 0 for off, 1 for on
// 0 for not trying to stop, 1 for trying to stop (change back to 0 after stopped)
static int stop = 0; 
static int alreadySlept = 0;

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

enum state {OFFLINE, IDLE, LOADING, UP, DOWN}; 

static struct proc_dir_entry *proc_entry;

struct Passenger // struct to store all of the varibles that passenger needs
{
    int weight; 
    int start_floor;
    int destination_floor;
    int type; 
    struct list_head list; // connect to next passenger waiting on a floor (linked list)
};

struct Elevator
{
    int passengersServiced;
    enum state status;
    int current_floor;
    int numPassengers;
    int weight;
    struct task_struct *kthread; // struct to make a kthread to control elevator movement
    struct Passenger passengers[MAX_PASSENGERS]; // array of 5 passengers for inside the elevator
};

struct Building
{
    int num_people; // how many people waiting in the building 
    // each spot of array holds the first passenger on each floor
    struct Passenger passengersWaiting[NUM_FLOORS]; 
};

// this returns how many floors you need to move up/down (not just 1/-1)
int moveLogic(struct Elevator *e_thread)
{ 
    mutex_lock(&elevator_mutex);
    int currentLoc = e_thread->current_floor;
    mutex_unlock(&elevator_mutex);
    
    int closest = NUM_FLOORS + 1; // how far away is the closest floor to drop someone off
    
    for(int i = 0; i < MAX_PASSENGERS; i++)
    {
        mutex_lock(&elevator_mutex);
        int temp = (e_thread->passengers[i].type == -1);
        mutex_unlock(&elevator_mutex);
        
        if(temp) // we found a dummy passenger
        {
            continue;
        }
        else
        {
            mutex_lock(&elevator_mutex);
            // get distance this person needs to travel
            int distance = e_thread->passengers[i].destination_floor - currentLoc; 
            mutex_unlock(&elevator_mutex);
            
            if(abs(distance) < abs(closest))
            {
                closest = distance;
            }
        }
    }
    return closest;
}

int start_elevator(void)
{
    mutex_lock(&elevator_mutex);
    stop = 0; // no longer trying to stop, already has stopped
    
    if(flag == 1)
    {
        mutex_unlock(&elevator_mutex);
        return 1; // elevator is already active so return 1
    }

    // if flag is 0
    flag = 1; 
    elevator_thread.status = IDLE; 
    mutex_unlock(&elevator_mutex);
    
    return 0;
}

int issue_request(int start_floor, int destination_floor, int type)
{    
    mutex_lock(&floor_mutex);
    
    // kmalloc because making a new passenger and need to know we have memory to do so
    struct Passenger *new_passenger = kmalloc(sizeof(struct Passenger), GFP_KERNEL); 
    
    if(!new_passenger)
    {
        printk(KERN_INFO "Error: could not allocate memory for new passenger\n");
        mutex_unlock(&floor_mutex);
        return -ENOMEM;
    }

    // initialize new passengers
    new_passenger->type = type;
    new_passenger->weight = 100 + (type * 50);
    new_passenger->start_floor = start_floor;
    new_passenger->destination_floor = destination_floor;

    building.num_people += 1;
    
    // add passenger with list_add_tail
    list_add_tail(&new_passenger->list, &building.passengersWaiting[start_floor-1].list); 

    mutex_unlock(&floor_mutex);

    return 0;
}

int stop_elevator(void)
{
    // not stopping already
    if(stop == 0)
    {
        stop = 1; // indicate stop picking up new passengers
        return 0;
    }

    // already in the process of stopping
    return 1;
}

// this adds passenger to elevator, not creating new passenger
void addPassenger(struct Elevator *e_thread)
{
    int done = 0;
    
    while((!done)) // can fit another passenger
    { 
        mutex_lock(&elevator_mutex);
        int temp = (e_thread->numPassengers < MAX_PASSENGERS);
        mutex_unlock(&elevator_mutex);
        
        if(temp) // can fit more passengers inside elevator
        { 
            mutex_lock(&floor_mutex);
            mutex_lock(&elevator_mutex);
            int temp0 = 
                (list_empty(&building.passengersWaiting[(e_thread->current_floor)-1].list) != 0);
            mutex_unlock(&elevator_mutex);
            mutex_unlock(&floor_mutex);

            if(temp0)
            {
                break; // leave while loop - no one waiting on floor
            }

            mutex_lock(&elevator_mutex);
            mutex_lock(&floor_mutex);
            struct Passenger *waiting_passenger = list_first_entry(
                            &building.passengersWaiting[e_thread->current_floor - 1].list, 
                            struct Passenger, list);
            int temp5 = ((e_thread->weight + waiting_passenger->weight) <= MAX_WEIGHT);
            mutex_unlock(&floor_mutex);
            mutex_unlock(&elevator_mutex);
            
            if(temp5) // weight is okay
            {
                for(int i = 0; i < MAX_PASSENGERS; i++)
                {
                    mutex_lock(&elevator_mutex);
                    mutex_lock(&floor_mutex);
                    if(e_thread->passengers[i].type == -1)
                    {
                        if(alreadySlept == 0)
                        {
                            mutex_unlock(&elevator_mutex);
                            mutex_unlock(&floor_mutex);
                            ssleep(1); // this is where it sleeps for 1 sec per passenger - we want 1 sec total
                            mutex_lock(&elevator_mutex);
                            mutex_lock(&floor_mutex);
                            alreadySlept = 1;
                        }
                        
                        struct Passenger *waiting_passenger = list_first_entry(
                            &building.passengersWaiting[e_thread->current_floor - 1].list, 
                            struct Passenger, list);
                       
                        e_thread->passengers[i].type = waiting_passenger->type; 
                        e_thread->passengers[i].destination_floor = 
                            waiting_passenger->destination_floor;
                        e_thread->passengers[i].start_floor = waiting_passenger->start_floor;
                        e_thread->passengers[i].weight = waiting_passenger->weight;
                        e_thread->numPassengers++;
                        
                        // deletes head because they are now inside elevator, no longer on floor
                        list_del(&waiting_passenger->list);
                        building.num_people--;
                        e_thread->weight += waiting_passenger->weight;
                        i = MAX_PASSENGERS; // instead of break
                    } 
                    mutex_unlock(&elevator_mutex);
                    mutex_unlock(&floor_mutex);
                }
            }
            else
                break; // next person is too heavy so break out of while loop 
        }
        else 
        {
            done = 1; 
        }
    }
}         
        
void removePassenger(struct Elevator *e_thread)
{
    //ssleep(1);

    for(int i = 0; i < MAX_PASSENGERS; i++)
    {
        mutex_lock(&elevator_mutex);
        int temp = ((e_thread->passengers[i].destination_floor == e_thread->current_floor) && 
            (e_thread->passengers[i].type != -1));
        mutex_unlock(&elevator_mutex);
        
        if(temp) // if there is someone to remove
        {
            if(alreadySlept == 0)
            {
                ssleep(1); // this is where it sleeps for 1 sec per passenger - we want 1 sec total
                alreadySlept = 1;
            }
            
            mutex_lock(&elevator_mutex);
            e_thread->weight -= e_thread->passengers[i].weight;
            e_thread->passengers[i].type = -1; 
            e_thread->numPassengers--;
            e_thread->passengersServiced++;
            mutex_unlock(&elevator_mutex);
        }
    }
}

int move_to_next_floor(struct Elevator *e_thread, int num)
{
    // -1 for down
    // 1 for up
    int newFloor = e_thread->current_floor + num;
    
    if((newFloor <= 0) || (newFloor > NUM_FLOORS))
        return 0;

    // else
    e_thread->current_floor = newFloor;
    return 1;
}

void process_elevator_state(struct Elevator *e_thread) 
{
    mutex_lock(&elevator_mutex);
    int temp = e_thread->status;
    mutex_unlock(&elevator_mutex);
    
    switch(temp) 
    {
        case UP:
            ssleep(2);
           
            mutex_lock(&elevator_mutex);
            move_to_next_floor(e_thread, 1); // only move if not at floor 6
            e_thread->status = LOADING;
            mutex_unlock(&elevator_mutex);
           
            break;
        case DOWN:
            ssleep(2); 
            
            mutex_lock(&elevator_mutex);
            move_to_next_floor(e_thread, -1);
            e_thread->status = LOADING;
            mutex_unlock(&elevator_mutex);

            break;
        case IDLE: // elevator starts here  and is also when there are no more passengers waiting
            // check if there are passengers waiting and if so then make it not idle anymore
            mutex_lock(&floor_mutex);
            if(stop == 1)
            {
                e_thread->status = OFFLINE;
                flag = 0;
                stop = 0;
            }
            
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
            alreadySlept = 0;

            // unload and then load passengers
            removePassenger(e_thread); // this removes (unloads) and does ssleep(1)

            // load the passengers
            if(stop == 0) // Not in the process of trying to stop
                addPassenger(e_thread);

            int dir = moveLogic(e_thread);
            
            mutex_lock(&elevator_mutex);
            int temp2 = (e_thread->numPassengers == 0);
            mutex_unlock(&elevator_mutex);
        
            if(temp2) // if nobody in the elevator
            {
                if(stop == 1) // emptied elevator - can stop it - stop_elevator() called earlier 
                // reminder that if stop = 1 that means you are in the PROCESS of trying to stop
                {
                    mutex_lock(&elevator_mutex);
                    e_thread->status = OFFLINE; // sits on a floor until start_elevator is called
                    flag = 0; // flag shows that elevator is stopped
                    stop = 0; // no longer trying to stop, it is stopped
                    mutex_unlock(&elevator_mutex);
                }
                else
                {
                    if(building.num_people == 0)
                    {
                        mutex_lock(&elevator_mutex);
                        e_thread->status = IDLE;
                        mutex_unlock(&elevator_mutex);
                    }
                    else
                    {
                        // find waiting passengers on other floors
                            // no one in elevator and no one on this floor
                        mutex_lock(&elevator_mutex);
                        int direction = NUM_FLOORS+1; 
                        int currentLoc = e_thread->current_floor;
                        mutex_unlock(&elevator_mutex);

                        for(int i = 0; i < NUM_FLOORS; i++)
                        {
                            mutex_lock(&floor_mutex);
                            // if waiting passengers head != NULL
                            // not empty return 0 - if people waiting on floor
                            if(list_empty(&building.passengersWaiting[i].list) == 0) 
                            {
                                struct Passenger *waiting_passenger = list_first_entry(
                                    &building.passengersWaiting[i].list, struct Passenger, list); 
                                
                                int distance = waiting_passenger->start_floor - currentLoc;
                                
                                if(abs(distance) < abs(direction)) 
                                {
                                    direction = distance; 
                                }
                            }
                            mutex_unlock(&floor_mutex);
                        }
                        if(direction < 0) // people waiting on floor below
                        {
                            mutex_lock(&elevator_mutex);
                            e_thread->status = DOWN;
                            mutex_unlock(&elevator_mutex);
                        }
                        else
                        {
                            mutex_lock(&elevator_mutex);
                            e_thread->status = UP; 
                            mutex_unlock(&elevator_mutex);
                        }
                    }
                }
            }
            // if  need to go up:
            else if(dir > 0)
            {
                mutex_lock(&elevator_mutex);
                e_thread->status = UP;
                mutex_unlock(&elevator_mutex);
            } 
            else // if need to go down:
            {
                mutex_lock(&elevator_mutex);
                e_thread->status = DOWN; 
                mutex_unlock(&elevator_mutex);
            }
            break;
        default:
            break;
    }
}

// function to print to proc file the bar state using waiter state
int print_building_state(char *buf, struct Elevator *e_thread) 
{
    int len = 0;

    // convert enums (integers) to strings
    const char *status[5] = {"OFFLINE", "IDLE", "LOADING", "UP", "DOWN"};

    mutex_lock(&elevator_mutex);
    len = sprintf(buf, "Elevator state: %s\n", status[e_thread->status]);
    len += sprintf(buf + len, "Current floor: %d\n", e_thread->current_floor);
    len += sprintf(buf + len, "Current weight: %d\n", e_thread->weight);
    len += sprintf(buf + len, "Elevator status:"); // This is who is inside the elevator
    mutex_unlock(&elevator_mutex);
    
    for(int i = 0; i < MAX_PASSENGERS; i++)
    {
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

    for(int i = NUM_FLOORS-1; i >= 0; i--) 
    {
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
        struct Passenger *variable;
        
        mutex_lock(&floor_mutex);
        int temp3 = (list_empty(&building.passengersWaiting[i].list) == 0);
        mutex_unlock(&floor_mutex);
        
        if(temp3)
        { // if not empty
            int total = 0;
           
            mutex_lock(&floor_mutex);
            list_for_each_entry(variable, &building.passengersWaiting[i].list, list)
            {
                total++;
            }
            mutex_unlock(&floor_mutex);
           
            len += sprintf(buf + len, " %d", total);

            mutex_unlock(&elevator_mutex);
            mutex_lock(&floor_mutex);
            // go thru list of passengers waiting on floor i
            list_for_each_entry(variable, &building.passengersWaiting[i].list, list)
            { 
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
    len += sprintf(buf + len, "Number of passengers serviced: %d\n", 
        e_thread->passengersServiced);
    mutex_unlock(&floor_mutex);

    return len;
}

static ssize_t procfile_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{ 
    
    char buf[10000];
    int len = 0;

    if (*ppos > 0 || count < 10000)
        return 0;

    len = print_building_state(buf, &elevator_thread); // this is how we write to the file!

    return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}

static struct proc_ops procfile_pops = {
	.proc_read = procfile_read,
};

int elevator_active(void *_elevator) 
{
    struct Elevator *e_thread = (struct Elevator *) _elevator;

    while(!kthread_should_stop()) 
    {
        process_elevator_state(e_thread);
    }

    return 0;
}

int spawn_elevator(struct Elevator *e_thread) 
{
    e_thread->kthread =
        kthread_run(elevator_active, e_thread, "thread elevator\n"); // thread spawns here

    return 0;
}

static int __init elevator_init(void)
{ 
    // This is where we link our system calls to our stubs
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;

    for(int i = 0; i < NUM_FLOORS; i++)
    {
        mutex_lock(&floor_mutex);
        INIT_LIST_HEAD(&building.passengersWaiting[i].list);
        mutex_unlock(&floor_mutex);
    }

    spawn_elevator(&elevator_thread);
    
    mutex_lock(&elevator_mutex);
    static int current_floor = 1; 
    elevator_thread.current_floor = current_floor;
    elevator_thread.status = OFFLINE; 
    elevator_thread.numPassengers = 0; 
    elevator_thread.passengersServiced = 0;
    mutex_unlock(&elevator_mutex);

    for(int i = 0; i < MAX_PASSENGERS; i++) // This for initializing passengers INSIDE elevator
    {
        mutex_lock(&elevator_mutex);
        elevator_thread.passengers[i].type = -1;
        mutex_unlock(&elevator_mutex);
    }

    if(IS_ERR(elevator_thread.kthread)) 
    {
        printk(KERN_WARNING "error creating thread");
        remove_proc_entry(ENTRY_NAME, PARENT);
        return PTR_ERR(elevator_thread.kthread);
    }

    proc_entry = proc_create( // this is where we create the proc file
        ENTRY_NAME,
        PERMS,
        PARENT,
        &procfile_pops
    );

    return 0;
}

static void __exit elevator_exit(void)
{
    struct Passenger *passenger, *next;

    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;

    for(int i = 0; i < NUM_FLOORS; i++)
    {
        mutex_lock(&floor_mutex);
        list_for_each_entry_safe(passenger, next, &building.passengersWaiting[i].list, list) 
        {
            list_del(&passenger->list);
            kfree(passenger);
        }
        mutex_unlock(&floor_mutex);
    }    

    // Free the memory we allocated in init
    for(int i = 0; i < MAX_PASSENGERS; i++)
    {
        mutex_lock(&elevator_mutex);
        kfree(&elevator_thread.passengers[i]);
        mutex_unlock(&elevator_mutex);
    }

    kthread_stop(elevator_thread.kthread); // this is where we stop the thread.

    remove_proc_entry(ENTRY_NAME, PARENT);
}

module_init(elevator_init);
module_exit(elevator_exit);
