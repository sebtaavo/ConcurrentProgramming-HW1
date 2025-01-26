# ConcurrentProgramming-HW1
KTH, Concurrent Programming ID1217, VT25

### Authors:
Sebastian Taavo Ek
Simon Lieb Fredriksson


## Contents
 
The two files in this repository represent task 1 and task 2 respectively in Homework 1: Critical Sections. Locks, Barriers, and Condition Variables. 

### matrixMath.c

The skeleton of the code in matrixMath.c was first written by and supplied by course representatives, before later being modified by our group.
The program initializes a matrix of a given size and populates it with random numbers modulo that same size. It then uses multithreading with
a number of threads equal to the input argument to process the matrix and find the sum of all elements, as well as the minimum and maximum value
in the matrix. Finally, it prints the elapsed time to process the whole matrix to the standard output as a way of learning more about how the
number of threads effects performance and efficiency.

### quickSort.c

This file was created by our group from scratch, while using a lot of the concepts and some of the skeleton code from matrixMath.c. In particular
when it comes to pthread creation and mutex locking. The file generates an array of a given size and then uses a recursive quicksort algorithm
to partition and sort its elements. At each partition, we create and assign a new thread to the left and right branches of the tree-like
process. If we've reached the maximum amount of threads (tracked by using a mutex lock on a tracking variable which increments on each creation
and decrements on each join) we then simply delve down the branches using the same thread. We finally measure the time elapsed and print it
to the standard output.
