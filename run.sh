#!/bin/bash

g++ -std=c++14 bot.cpp -o bot.exe

while true; do 
    ./bot.exe; 
    sleep 1; 
done
