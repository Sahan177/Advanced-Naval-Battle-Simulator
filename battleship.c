#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    char type;
    double x;
    double y;
    double vmax;
    double angle;
    int alive;
} Battleship;

typedef struct {
    int id;
    char type;
    double x;
    double y;
    double vmin;
    double vmax;
    double angleMin;
    double angleMax;
    double impactPower;
    int alive;
} Escort;

void setEscortProperties(Escort *escort) {
    switch (escort->type) {
        case 'A':
            escort->angleMin = 20.0;
            escort->impactPower = 0.08;
            break;
        case 'B':
            escort->angleMin = 30.0;
            escort->impactPower = 0.06;
            break;
        case 'C':
            escort->angleMin = 25.0;
            escort->impactPower = 0.07;
            break;
        case 'D':
            escort->angleMin = 50.0;
            escort->impactPower = 0.05;
            break; // Added missing break here
        case 'E':
            escort->angleMin = 70.0;
            escort->impactPower = 0.04;
            break;
        default:
            escort->angleMin = 0.0;
            escort->impactPower = 0.0;
            break;
    }
}

int main() {
    double battlefieldSize;
    int numberOfEscorts;
    Battleship battleship;
    Escort *escorts;

    srand(time(NULL));

    printf("========================================\n");
    printf("        ADVANCED NAVAL BATTLE\n");
    printf("========================================\n\n");

    printf("Enter battlefield dimension: ");
    if (scanf("%lf", &battlefieldSize) != 1) return 1;

    printf("Enter number of Escort ships: ");
    if (scanf("%d", &numberOfEscorts) != 1) return 1;

    escorts = malloc(numberOfEscorts * sizeof(Escort));
    if (escorts == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    // Example loop to properly initialize escorts and fix the uninitialized bug
    char availableTypes[] = {'A', 'B', 'C', 'D', 'E'};
    for (int i = 0; i < numberOfEscorts; i++) {
        escorts[i].id = i + 1;
        escorts[i].type = availableTypes[rand() % 5]; // Assign a type first
        escorts[i].x = ((double)rand() / RAND_MAX) * battlefieldSize;
        escorts[i].y = ((double)rand() / RAND_MAX) * battlefieldSize;
        escorts[i].alive = 1;
        
        // Now it's safe to call properties setup
        setEscortProperties(&escorts[i]);
    }

    printf("\nChoose Battleship type:\n");
    printf("U - USS Iowa\n");
    printf("M - MS King George V\n");
    printf("R - Richelieu\n");
    printf("S - Sovetsky Soyuz-class\n");
    printf("\nEnter choice: ");
    scanf(" %c", &battleship.type);

    battleship.x = ((double)rand() / RAND_MAX) * battlefieldSize;
    battleship.y = ((double)rand() / RAND_MAX) * battlefieldSize;
    battleship.alive = 1;

    printf("\nBattleship created!\n");
    printf("Battleship type: %c\n", battleship.type);
    printf("Battleship position: (%.2f, %.2f)\n", battleship.x, battleship.y);

    // Free allocated memory before exiting
    free(escorts);

    return 0;
}

