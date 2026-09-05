#include <stdio.h>
#include <stdlib.h>
#include <time.h>


typedef struct
{
    char type;
    double x;
    double y;
    double vmax;
    double angle;
    int alive;
} Battleship;

typedef struct
{
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

int main()
{
    double battlefieldSize;

    int numberOfEscorts;

    Battleship battleship;

    Escort *escorts;

    srand(time(NULL));

    printf("========================================\n");
    printf("       ADVANCED NAVAL BATTLE\n");
    printf("========================================\n\n");

    printf("Enter battlefield dimension: ");
    scanf("%lf", &battlefieldSize);

    printf("Enter number of Escort ships: ");
    scanf("%d", &numberOfEscorts);

    escorts = malloc(numberOfEscorts * sizeof(Escort));

    if (escorts == NULL)
    {
    printf("Memory allocation failed.\n");
    return 1;
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
    printf("Battleship position: (%.2f, %.2f)\n",
           battleship.x, battleship.y);

    return 0;
}






















































































































































































































