//__________________________________ PAT Speaking: The RGBLED driver can indicate each color on the LED with this -> RGB_Indicate(int red, int green, int blue) function __________________________________
#include <wiringPi.h>
#include <csignal>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstdio>
#include "json.hpp" 
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <sched.h>
#include "pat_utilities.h"



// GPIO Pins
#define BPIN 20
#define GPIN 22
#define RPIN 23

// Global state
volatile unsigned int counter = 0;
volatile unsigned int pwmCounter = 0;
RequestData request(_path_nodejs);

void digitalWriteRGB(bool r, bool g, bool b) {
    // LOG_PRINTF("%d %d %d\n", r, g, b);
    digitalWrite(RPIN, r);
    digitalWrite(GPIN, g);
    digitalWrite(BPIN, b);
}

bool writeRequestToFile(string command , int red, int green, int blue, string updated) {
    Json doc = readJson(_path_cpp);
    Json request = {{"command", command}, {"red", red}, {"green", green}, {"blue", blue}, {"updated", updated} };   
    doc["request"]=request;
    return replaceJsonFile(_path_cpp, doc);
}

bool writeResponseToFile(string command , int red, int green, int blue) {
    Json doc = readJson(_path_cpp);
    Json response = {{"command", command}, {"red", red}, {"green", green}, {"blue", blue}, {"updated", getTimestamp()} };   
    doc["response"]=response;
    return replaceJsonFile(_path_cpp, doc);
}


volatile bool redHandler = false;
volatile bool greenHandler = false;
volatile bool blueHandler = false;


volatile uint8_t redRound = 0;
volatile uint8_t greenRound = 0;
volatile uint8_t blueRound = 0;
#define _cofRound 16

void timerHandler(int signum) {    

    pwmCounter = ++pwmCounter & 0xF;

    if(redHandler != (pwmCounter < redRound ? HIGH : LOW)){
        redHandler = !redHandler;
        digitalWrite(RPIN, redHandler);
    }

    if(greenHandler != (pwmCounter < greenRound ? HIGH : LOW)){
        greenHandler = !greenHandler;
        digitalWrite(GPIN, greenHandler);
    }

    if(blueHandler != (pwmCounter < blueRound ? HIGH : LOW)){
        blueHandler = !blueHandler;
        digitalWrite(BPIN, blueHandler);
    }

}


void setRealtimePriority() {
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);

    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        LOG_PERROR("sched_setscheduler failed");
        exit(EXIT_FAILURE);
    } else {
        // LOG_COUT << "[+] Real-time priority set: " << param.sched_priority << std::endl;
    }
}

void initTimer(int microseconds) {
    redRound = request.red / _cofRound;
    greenRound = request.green / _cofRound;
    blueRound = request.blue / _cofRound;

    signal(SIGALRM, timerHandler);

    struct itimerval timer = {};
    timer.it_value.tv_sec = microseconds / 1000000;
    timer.it_value.tv_usec = microseconds % 1000000;
    timer.it_interval = timer.it_value;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        LOG_PERROR("setitimer failed");
        exit(EXIT_FAILURE);
    }

    setRealtimePriority();
}

void stopTimer() {
    // Disable the timer by setting both intervals to 0
    struct itimerval timer = {};
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        LOG_PERROR("Failed to disable timer");
    }

    // Restore scheduler to default (usually SCHED_OTHER with priority 0)
    struct sched_param param = {0};
    if (sched_setscheduler(0, SCHED_OTHER, &param) == -1) {
        LOG_PERROR("Failed to reset scheduler policy");
    }
}



void handleSignal(int signal) {  
    stopTimer();  
    digitalWriteRGB(0, 0, 0);
    writeResponseToFile("error",0, 0, 0);              
    LOG_PRINTF("handleSignal\n");
    exit(1);
}

void setupSignalHandlers() {
    // Common termination signals
    signal(SIGINT, handleSignal);    // Ctrl+C
    signal(SIGTERM, handleSignal);   // kill <pid>
    signal(SIGHUP, handleSignal);    // Terminal closed
    signal(SIGQUIT, handleSignal);   // Ctrl+\

    // Crash-related signals
    signal(SIGABRT, handleSignal);   // abort()
    signal(SIGFPE,  handleSignal);   // Divide by zero, floating-point error
    signal(SIGILL,  handleSignal);   // Illegal instruction
    signal(SIGSEGV, handleSignal);   // Segmentation fault
    signal(SIGBUS,  handleSignal);   // Bus error

    // Optional (not always useful but included for completeness)
    signal(SIGTRAP, handleSignal);   // Debug trap
}

bool validateRGBJson() {
    Json doc = readJson(_path_nodejs);

    if (doc.find("command") == doc.end()) {
        LOG_CERR << "Error: 'command' key missing\n";
        return false;
    }

    if (doc.find("red") == doc.end()) {
        LOG_CERR << "Error: 'red' key missing\n";
        return false;
    }
    if (doc.find("green") == doc.end()) {
        LOG_CERR << "Error: 'green' key missing\n";
        return false;
    }
    if (doc.find("blue") == doc.end()) {
        LOG_CERR << "Error: 'blue' key missing\n";
        return false;
    }
    if (doc.find("updated") == doc.end()) {
        LOG_CERR << "Error: 'updated' key missing\n";
        return false;
    }

    return  true;
}



int main(void) {  

    if (wiringPiSetup() == -1) return 1; 

    if(!killDuplicateProcesses(_driver_name)){
        LOG_PERROR("Error: could not kill Duplicate Processes\n");
    }
    pinMode(RPIN, OUTPUT);
    pinMode(GPIN, OUTPUT);
    pinMode(BPIN, OUTPUT);

    setupSignalHandlers();


    //--------------------------------------------------------------------------------------------
    
    if(!validateRGBJson() || isRecentlyBooted()){        
        Json doc = {{"command", "idle"}, {"red", 0}, {"green", 0}, {"blue", 0} ,{"updated",getTimestamp()}};     
        replaceJsonFile(_path_nodejs, doc);
        LOG_PRINTF("Recently Booted or RGB Json is not valid \n");
    }   
    LOG_PRINTF("RGB driver start\n");
    //--------------------------------------------------------------------------------------------
    while (1) {
        if(!killDuplicateProcesses(_driver_name)){
        LOG_PERROR("Error: could not kill Duplicate Processes\n");
        exit(1);
        }
        if(request.changed()){

        writeRequestToFile( request.command , request.red , request.green , request.blue , request.updated );
        if(request.command == "indicate"){
    //--------------------------------------------------------------------------------------------
        if ((request.red == 0 || request.red == 255) &&
            (request.green == 0 || request.green == 255) &&
            (request.blue == 0 || request.blue == 255)) {
                stopTimer();
                // LOG_PRINTF("RGB easy mode\n");

            if (writeResponseToFile("indicate",request.red, request.green, request.blue)) {
                digitalWriteRGB(bool(request.red), bool(request.green), bool(request.blue));
                LOG_PRINTF("Red = %d, Green = %d, Blue = %d\n", request.red, request.green, request.blue);
            }
            else if (writeResponseToFile("indicate",request.red, request.green, request.blue)) {
                digitalWriteRGB(bool(request.red), bool(request.green), bool(request.blue));
                LOG_PRINTF("Red = %d, Green = %d, Blue = %d\n", request.red, request.green, request.blue);
            }
            else {
                writeResponseToFile("error",0, 0, 0);
                digitalWriteRGB(0,0,0);
                LOG_PRINTF("could not write to file\n");
                LOG_PERROR("Error: could not write to file\n");
                exit(1);  
            }
        }
    //-------------------------------------------------------------------------------------------- 
        else if (writeResponseToFile("indicate",request.red, request.green, request.blue)) {
                LOG_PRINTF("Red = %d, Green = %d, Blue = %d\n", request.red, request.green, request.blue);
                initTimer(500);
            }
            else if (writeResponseToFile("indicate",request.red, request.green, request.blue)) {
                LOG_PRINTF("Red = %d, Green = %d, Blue = %d\n", request.red, request.green, request.blue);
                initTimer(500);
            }
            else
            {
                stopTimer();
                writeResponseToFile("error",0, 0, 0);                  
                digitalWriteRGB(0,0,0); 
                LOG_PRINTF("could not write to file\n");
                LOG_PERROR("Error: could not write to file\n");
                exit(1);  
            }
    //-------------------------------------------------------------------------------------------- 
        }else{
                writeResponseToFile("idle",0, 0, 0);
                stopTimer();
                digitalWriteRGB(0,0,0);
                LOG_PRINTF("Red = 0, Green = 0, Blue = 0\n");           
        }
    }
        msleep(200);
        
    }
    return 0;
}
