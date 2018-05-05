/*  Name: Joseph Park
 *    PS: 1415574
 *   Due: 4/30/2018
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <pthread.h>
#include <vector>
#include <unistd.h>
#include <functional>

struct carInfo;
static char tunnelStatus; // 'W' = To Whittier. 'B' = To Bear Valley. 'C' = Closed.
static int maxCars;
static int totalCars;
static int currCars;
static int numBBCars = 0;
static int numWBCars = 0;
static int numWaitCars = 0;
static pthread_mutex_t traffic_lock;
static pthread_cond_t wake_up;
static std::vector<carInfo> carVec;
static bool finished = false;
static int currCarNum = 0;
static pthread_t tunnelID;

struct carInfo{
    int carID;
    int arrivalTime;
    std::string destination; // WB = Whittier Bound. BB = Bear Valley Bound.
    int crossingTime;

    carInfo(int ID, int arrive, std::string dest, int cross){
        carID = ID;
        arrivalTime = arrive;
        destination = dest;
        crossingTime = cross;
    }
};

void *car(void *arg)
{
    pthread_t *tid = (pthread_t *)arg;
    carInfo currCar = carVec[currCarNum++];
    sleep(currCar.arrivalTime);

    if(currCarNum < carVec.size()) { // create next car thread
        pthread_create(&tid[currCarNum], NULL, car, (void *) tid);
    }

    std::string dest;
    if(currCar.destination == "WB") {
        dest = "Whittier";
        numWBCars++;
    }
    else {
        dest = "Bear Valley";
        numBBCars++;
    }
    std::cout << "Car #" << currCar.carID << " going to " << dest <<" arrives at the tunnel" << std::endl;

    pthread_mutex_lock(&traffic_lock);

    while(tunnelStatus != dest[0]){
        pthread_cond_wait(&wake_up, &traffic_lock);
    }
    if(currCars >= maxCars && tunnelStatus == dest[0]) {
        numWaitCars++;
//        std:: cout << "Delayed Car #" << currCar.carID << std::endl;
    }

    currCars++;
    std::cout << "Car #" << currCar.carID << " going to " << dest <<" enters the tunnel" << std::endl;
    pthread_mutex_unlock(&traffic_lock);
    sleep(currCar.crossingTime);
    pthread_mutex_lock(&traffic_lock);
    std::cout << "Car #" << currCar.carID << " going to " << dest <<" exits the tunnel" << std::endl;
    currCars--; totalCars--;
    pthread_mutex_unlock(&traffic_lock);
    pthread_cond_broadcast(&wake_up);

    if(totalCars == 0){
        finished = true; // not really necessary, but I'll leave it just in case.
        pthread_cancel(tunnelID);
    }
    pthread_exit(arg);
}

void *tunnel(void *arg)
{
    while(!finished){
        pthread_mutex_lock(&traffic_lock);
        tunnelStatus = 'W';
        std::cout << "The tunnel is now open to Whittier-bound traffic." << std::endl;
        pthread_cond_broadcast(&wake_up);
        pthread_mutex_unlock(&traffic_lock);
        sleep(5);

        pthread_mutex_lock(&traffic_lock);
        tunnelStatus = 'C';
        std::cout << "The tunnel is now closed to ALL traffic." << std::endl;
        pthread_mutex_unlock(&traffic_lock);
        sleep(5);

        pthread_mutex_lock(&traffic_lock);
        tunnelStatus = 'B';
        std::cout << "The tunnel is now open to Bear Valley-bound traffic." << std::endl;
        pthread_cond_broadcast(&wake_up);
        pthread_mutex_unlock(&traffic_lock);
        sleep(5);

        pthread_mutex_lock(&traffic_lock);
        tunnelStatus = 'C';
        std::cout << "The tunnel is now closed to ALL traffic." << std::endl;
        pthread_mutex_unlock(&traffic_lock);
        sleep(5);
    }
}

void testStoreInput(std::vector<carInfo> carVec){
    std::cout << maxCars << "\n";
    for (auto &i : carVec) {
        std::cout << i.carID << " " << i.arrivalTime << " " << i.destination << " " << i.crossingTime << "\n";
    }
}

void printSummary(){
    std::cout << numBBCars << " car(s) going to Bear Valley arrived at the tunnel.\n";
    std::cout << numWBCars << " car(s) going to Whittier arrived at the tunnel.\n";
    std::cout << numWaitCars << " car(s) were delayed.\n";
}

int main() {
//    store input (using I/O redirection)
    int carNum = 1;
    int arrivalTime, crossingTime;
    std::string dest;

    std::cin >> maxCars;
    while(std::cin >> arrivalTime >> dest >> crossingTime){
        carVec.emplace_back(carNum++, arrivalTime, dest, crossingTime);
        ++totalCars;
    }
//    testStoreInput(carVec);

    pthread_mutex_init(&traffic_lock, nullptr);
    pthread_cond_init(&wake_up, NULL);

    pthread_t tid[totalCars];
    pthread_create(&tunnelID, NULL, tunnel, NULL);

    pthread_create(&tid[0], NULL, car, (void *)tid);
    pthread_join(tid[0], nullptr);
    pthread_join(tunnelID, nullptr);

    printSummary();
}