#include <stdio.h>
#include "relief.h"


int calculateReliefValue(int urgency, int needSeverity)
{
    return urgency * needSeverity;
}


void inputShelter(Shelter *shelter)
{
    printf("\n--- Enter Shelter Details ---\n");

    printf("Shelter ID: ");
    scanf("%d", &shelter->shelterID);

    printf("Shelter Name: ");
    scanf(" %[^\n]", shelter->name);

    printf("Population: ");
    scanf("%d", &shelter->population);

    printf("Urgency (1-10): ");
    scanf("%d", &shelter->urgency);
}


void inputPackages(
    SupplyPackage packages[],
    int n,
    int shelterUrgency
)
{
    for (int i = 0; i < n; i++)
    {
        printf("\n--- Package %d ---\n", i + 1);

        printf("Package ID: ");
        scanf("%d", &packages[i].packageID);

        printf("Supply Type: ");
        scanf(" %[^\n]", packages[i].type);

        printf("Weight (kg): ");
        scanf("%d", &packages[i].weight);

        printf("Need Severity (1-10): ");
        scanf("%d", &packages[i].needSeverity);

        packages[i].reliefValue =
            calculateReliefValue(
                shelterUrgency,
                packages[i].needSeverity
            );
    }
}


void displayPackages(
    SupplyPackage packages[],
    int n
)
{
    printf("\n--- Relief Packages ---\n");

    for (int i = 0; i < n; i++)
    {
        printf(
            "%d | %s | Weight: %d | Relief Value: %d\n",
            packages[i].packageID,
            packages[i].type,
            packages[i].weight,
            packages[i].reliefValue
        );
    }
}


void inputVehicle(Vehicle *vehicle)
{
    printf("\n--- Enter Vehicle Details ---\n");

    printf("Vehicle ID: ");
    scanf("%d", &vehicle->vehicleID);

    printf("Capacity (kg): ");
    scanf("%d", &vehicle->capacity);

    printf("Available? (1 = Yes, 0 = No): ");
    scanf("%d", &vehicle->available);
}