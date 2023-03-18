#include <fstream>
#include <iostream>
#include <sstream>
#include <queue>
#include <vector>
#include <algorithm>

using namespace std;

// data structs

struct Customer
{
    int id;             // customer id
    int arrival;        // arrival time
    int priority;       // customer priority (1-5), smaller -> higher
    int age;            // aging (0 or 1 in queue1, 0-95 in queue2)
    int tickets;        // required number of tickets

    int end;            // ticket process ending time
    int ready;          // customer ready time
    int running;        // ticket process running time
    int waiting;        // customer waiting time

    bool started;       // marked when a customer is processed
    int hidden;         // hidden priority, smaller -> higher

    // constructor
    Customer(int a, int b, int c, int d, int e)
    {
        id = a;
        arrival = b;
        priority = c;
        age = d;
        tickets = e;

        end = 0;
        ready = 0;
        running = 0;
        waiting = 0;

        started = false;
        hidden = 1;
    }
};

// Highest Priority First for queue1
struct HPF
{
    // operator overload
    bool operator() (Customer a, Customer b)
    {
        if(a.priority != b.priority) return a.priority > b.priority;
        if(a.hidden != b.hidden) return a.hidden > b.hidden;
        if(a.arrival != b.arrival) return a.arrival > b.arrival;
        return a.id > b.id;
    }
};

// Shortest Remaining Time First for queue2
struct SRF
{
    // operator overload
    bool operator() (Customer a, Customer b)
    {
        if(a.tickets != b.tickets) return a.tickets > b.tickets;
        if(a.priority != b.priority) return a.priority > b.priority;
        if(a.arrival != b.arrival) return a.arrival > b.arrival;
        if(a.hidden != b.hidden) return a.hidden > b.hidden;
        return a.id > b.id;
    }
};

// by arrival order (smallest first)
struct Arrival
{
    // operator overload
    bool operator() (Customer a, Customer b)
    {
        return a.arrival > b.arrival;
    }
};

// by ending order (smallest first)
struct End
{
    // operator overload
    bool operator() (Customer a, Customer b)
    {
        return a.end > b.end;
    }
};

// priority queues

priority_queue<Customer, vector<Customer>, HPF> queue1;
priority_queue<Customer, vector<Customer>, SRF> queue2;
priority_queue<Customer, vector<Customer>, End> results;

// global variables

int currentTime = 0;    // current time unit
int prevTime = 0;       // previous time unit
int maxHidden = 2;      // current hidden priority

// functions

// pick the next best customer from queue1
Customer nextCustomerQ1()
{
    priority_queue<Customer, vector<Customer>, Arrival> temp;
    Customer customer = queue1.top();
    bool flag = false;

    // look for the next available best customer
    while(!queue1.empty())
    {
        customer = queue1.top();
        queue1.pop();

        if(customer.arrival > currentTime)
        {
            // customer not arrived yet
            temp.push(customer);
        }
        else
        {
            // customer available
            flag = true;
            break;
        }
    }

    if(!flag)
    {
        // pick customer according to arrival time
        customer = temp.top();
        temp.pop();
    }

    // put customers back into queue1
    while(!temp.empty())
    {
        queue1.push(temp.top());
        temp.pop();
    }

    return customer;
}

// pick the next best customer from queue2
Customer nextCustomerQ2()
{
    priority_queue<Customer, vector<Customer>, Arrival> temp;
    Customer customer = queue2.top();
    bool flag = false;

    // look for the next available best customer
    while(!queue2.empty())
    {
        customer = queue2.top();
        queue2.pop();

        if(customer.arrival > currentTime)
        {
            // customer not arrived yet
            temp.push(customer);
        }
        else
        {
            // customer available
            flag = true;
            break;
        }
    }

    if(!flag)
    {
        // pick customer according to arrival time
        customer = temp.top();
        temp.pop();
    }

    // put customers back into queue2
    while(!temp.empty())
    {
        queue2.push(temp.top());
        temp.pop();
    }

    return customer;
}

// update arriving customers in queue1
void updateQ1()
{
    priority_queue<Customer, vector<Customer>, HPF> temp;

    
    while(!queue1.empty())
    {
        Customer customer = queue1.top();
        queue1.pop();

        // put new arriving customers to the end of queue
        if((customer.arrival <= currentTime) && (customer.hidden == 1))
        {
            customer.hidden = maxHidden;
            maxHidden ++;
        }

        temp.push(customer);
    }

    queue1 = temp;
}

// aging all waiting customers in queue2
bool agingQ2()
{
    priority_queue<Customer, vector<Customer>, SRF> temp;
    vector<Customer> promoted;
    bool isPromoted = false;

    while(!queue2.empty())
    {
        Customer customer = queue2.top();
        queue2.pop();

        if(customer.arrival < currentTime)
        {
            // update waiting time
            if(customer.arrival > prevTime)
            {
                customer.waiting += (currentTime - customer.arrival);
            }
            else
            {
                customer.waiting += (currentTime - prevTime);
            }

            if(customer.waiting >= 100)
            {
                // priority increase every 100 time steps
                customer.priority -= (customer.waiting / 100);
                customer.waiting %= 100;
            }

            if(customer.priority <= 3)
            {
                // promotion
                promoted.push_back(customer);
                isPromoted = true;
            }
            else
            {
                temp.push(customer);
            }
        }
        else
        {
            temp.push(customer);
        }
    }

    // sort promoted customers and push into queue1
    sort(promoted.begin(), promoted.end(), [](const Customer &a, const Customer &b)
		{return a.waiting > b.waiting;});
    for(int i = 0; i < promoted.size(); i++)
    {
        promoted[i].age = 0;
        promoted[i].hidden = maxHidden;
        maxHidden ++;
        queue1.push(promoted[i]);
        //printf("[%d]\tPromoted: a%d %d %d %d %d %d %d\n",currentTime,promoted[i].id,promoted[i].arrival,promoted[i].tickets,promoted[i].running,promoted[i].priority,promoted[i].age,promoted[i].hidden);
    }

    queue2 = temp;

    return isPromoted;
}

// process a customer in queue1 non-preemptively
void processQ1()
{
    Customer customer = nextCustomerQ1();
    int quota = ((10 - customer.priority) * 10) / 5;

    // update times
    if(customer.arrival > currentTime)
    {
        prevTime = currentTime;
        currentTime = customer.arrival;
    }

    //printf("[%d]\tQueue1: a%d %d %d %d %d %d %d\n",currentTime,customer.id,customer.arrival,customer.tickets,customer.running,customer.priority,customer.age,customer.hidden);

    // new arriving customer
    if(!customer.started)
    {
        customer.ready = currentTime;
        customer.started = true;
    }

    // tickets processing
    if(customer.tickets <= quota)
    {
        // finished within quota
        prevTime = currentTime;
        currentTime += (customer.tickets * 5);
        customer.running += (customer.tickets * 5);
        customer.end = currentTime;
        customer.waiting = customer.end - customer.ready - customer.running;
        results.push(customer);
        agingQ2();                          // age waiting customers in queue2
    }
    else
    {
        // required tickets more than quota
        prevTime = currentTime;
        currentTime += (quota * 5);
        customer.running += (quota * 5);
        customer.tickets -= quota;
        customer.age ++;
        updateQ1();                         // update new arriving customers in queue1
        agingQ2();                          // age waiting customers in queue2
        customer.hidden = maxHidden;        // lower hidden priority
        maxHidden ++;

        if(customer.age == 2)
        {
            customer.age = 0;
            customer.priority ++;
        }

        if(customer.priority == 4)
        {
            // demotion
            //printf("[%d]\tDemoted: a%d %d %d %d %d %d %d\n",currentTime,customer.id,customer.arrival,customer.tickets,customer.running,customer.priority,customer.age,customer.hidden);
            customer.age = 0;
            customer.waiting = 0;
            customer.hidden = 0;
            queue2.push(customer);
        }
        else
        {
            queue1.push(customer);
        }
    }
}

// process a customer in queue 2 preemptively
void processQ2()
{
    // process one ticket each cycle
    while(!queue2.empty())
    {
        // check if a new customer arrived in queue1
        if(!queue1.empty())
        {
            Customer temp = nextCustomerQ1();
            queue1.push(temp);
            if(temp.arrival <= currentTime) break;
        }

        Customer customer = nextCustomerQ2();
        customer.waiting = 0;
        
        // update times
        if(customer.arrival > currentTime)
        {
            prevTime = currentTime;
            currentTime = customer.arrival;
        }

        //printf("[%d]\tQueue2: a%d %d %d %d %d %d %d\n",currentTime,customer.id,customer.arrival,customer.tickets,customer.running,customer.priority,customer.age,customer.hidden);

        if(!customer.started)
        {
            // new arriving customer
            customer.ready = currentTime;
            customer.started = true;
        }

        if(customer.tickets == 0)
        {
            // finished processing
            customer.end = currentTime;
            customer.waiting = customer.end - customer.ready - customer.running;
            results.push(customer);
            break;
        }
        else
        {
            // process one ticket each cycle
            prevTime = currentTime;
            currentTime += 5;
            customer.running += 5;
            customer.tickets --;
            customer.age += 5;

            // age waiting customers
            if(agingQ2())
            {
                // promoted, end loop
                queue2.push(customer);
                break;
            }

            queue2.push(customer);
        }
    }
}

// read input data from file to priority queues
void input(char* path)
{
    ifstream inputFile(path);
    stringstream ss;

    for(string line; getline(inputFile, line);)
    {
        ss.clear();
        ss.str(line);

        // temp variables
        string id_str;
        int id, arrival, priority, age, tickets;

        // read from string stream
        ss >> id_str >> arrival >> priority >> age >> tickets;
        id = stoi(id_str.substr(1));

        // push into temp vectors according to priority
        if(priority <= 3)
        {
            queue1.push({id, arrival, priority, age, tickets});
        }
        else
        {
            queue2.push({id, arrival, priority, age, tickets});
        }
    }

    /*
    priority_queue<Customer, vector<Customer>, HPF> temp1 = queue1;
    priority_queue<Customer, vector<Customer>, SRF> temp2 = queue2;

    cout << "queue1" << endl;
    while(!queue1.empty())
    {
        printf("a%d %d %d %d %d\n",queue1.top().id,queue1.top().arrival,queue1.top().priority,queue1.top().age,queue1.top().tickets);
        queue1.pop();
    }

    cout << "queue2" << endl;
    while(!queue2.empty())
    {
        printf("a%d %d %d %d %d\n",queue2.top().id,queue2.top().arrival,queue2.top().priority,queue2.top().age,queue2.top().tickets);
        queue2.pop();
    }
    
    queue1 = temp1;
    queue2 = temp2;
    */

    inputFile.close();
}

// process tickets from priority queues
void process()
{
    // check if queues have customers
    while((!queue1.empty()) || (!queue2.empty()))
    {
        if(!queue1.empty() && queue2.empty())
        {
            // queue2 empty, process queue1
            processQ1();
        }
        else if(queue1.empty() && !queue2.empty())
        {
            // queue1 empty, process queue2
            processQ2();
        }
        else
        {
            // both queues have customers
            Customer a = nextCustomerQ1();
            Customer b = nextCustomerQ2();
            queue1.push(a);
            queue2.push(b);
            
            if(a.arrival > currentTime && b.arrival <= currentTime)
            {
                // customer in queue2 arrive earlier
                processQ2();
            }
            else if(a.arrival > currentTime && b.arrival > currentTime)
            {
                // compare arrival time
                if(a.arrival > b.arrival)
                {
                    processQ2();
                }
                else
                {
                    processQ1();
                }
            }
            else
            {
                processQ1();
            }
        }
    }
}

// print results sorted by ending time ascending order
void output()
{
    cout << "name   arrival   end   ready   running   waiting" << endl;
    while(!results.empty())
    {
        cout << "a" << results.top().id << "\t"
                    << results.top().arrival << "\t"
                    << results.top().end << "\t"
                    << results.top().ready << "\t"
                    << results.top().running << "\t"
                    << results.top().waiting << endl;
        results.pop();
    }
    cout << endl;
}

// main function
int main(int argc,char** argv)
{
    input(argv[1]);
    process();
    output();

    return 0;
}