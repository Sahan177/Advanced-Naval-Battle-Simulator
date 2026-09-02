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

