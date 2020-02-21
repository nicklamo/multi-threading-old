## Nicholas La Monica CSCI3753 Operating Systems

Multi-threading with the Pthreads API in C. This code looks up multiple IP addresses asynchronously based on there domain names.

### How to run the program:
1. Compile the code:
  ```
  make
  ```
2. Run the code with command line arguments:
  ```
  ./multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]
  ```
3. results.txt contains IP address to host name mapping. serviced.txt will have data on how many files each thread serviced
4. Clean object files when done:
  ```
  make clean
  ```
### Important files
1. multi-lookup.c multi-lookup.h
    - Contains all of the multi-threaded code and main driver function
2. util.c util.h
    - Actually performs the dnslookup
3. input/names*.txt
    - has all of the domain names you want to look up the IP addresses for
4. performance.py
    - python performance script
