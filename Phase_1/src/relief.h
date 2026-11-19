#ifndef RELIEF_H
#define RELIEF_H

#define MAX_PACKAGES 100

typedef struct
{
    int shelterID;
    char name[30];
    int population;
    int urgency;
} Shelter;

typedef struct
{
    int packageID;
    char type[30];
    int weight;
    int needSeverity;
    int reliefValue;
} SupplyPackage;

typedef struct
{
    int vehicleID;
    int capacity;
    int available;
} Vehicle;


// Relief-specific functions
int calculateReliefValue(int urgency, int needSeverity);

void inputShelter(Shelter *shelter);

void inputPackages(
    SupplyPackage packages[],
    int n,
    int shelterUrgency
);

void displayPackages(
    SupplyPackage packages[],
    int n
);

void inputVehicle(Vehicle *vehicle);

#endif