#include "validation.h"


int validatePopulation(int population)
{
    return population >= 0;
}


int validateUrgency(int urgency)
{
    return urgency >= 1 && urgency <= 10;
}


int validatePackageCount(int n)
{
    return n > 0 && n <= 100;
}


int validateWeight(int weight)
{
    return weight > 0;
}


int validateSeverity(int severity)
{
    return severity >= 1 && severity <= 10;
}


int validateCapacity(int capacity)
{
    return capacity > 0;
}


int validateAvailability(int available)
{
    return available == 0 || available == 1;
}