#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define GRAVITY 9.81
#define PI 3.14159265358979323846

// ==========================================
// DATA STRUCTURES
// ==========================================
typedef struct {
    double x, y;
} Position;

typedef struct {
    double x, y;
} PathPoint;

typedef struct {
    int id;
    char type_code; // 'A', 'B', 'C', 'D', 'E'
    Position pos;
    double v_min, v_max;
    double theta_L, theta_H;
    double impact_power;
    double r_min, r_max;
    int is_destroyed;
} EscortShip;

typedef struct {
    char type_code;
    Position pos;
    double v_max;
    double r_max;
} Battleship;

// ==========================================
// HELPER FUNCTIONS
// ==========================================
double get_distance(Position p1, Position p2) {
    return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

double calculate_range(double v, double theta_deg) {
    double theta_rad = theta_deg * (PI / 180.0);
    return (pow(v, 2) * sin(2 * theta_rad)) / GRAVITY;
}

void copy_escort_ships(EscortShip src[], EscortShip dest[], int count) {
    for (int i = 0; i < count; i++) {
        dest[i] = src[i];
    }
}

void generate_escort_ships(EscortShip esc_ships[], int count, double canvas_d, double v_max_b) {
    char types[5] = {'A', 'B', 'C', 'D', 'E'};
    double impact_powers[5] = {0.08, 0.06, 0.07, 0.05, 0.04};
    double angle_ranges[5] = {20.0, 30.0, 25.0, 50.0, 70.0};

    for (int i = 0; i < count; i++) {
        esc_ships[i].id = i;
        int type_idx = rand() % 5;
        esc_ships[i].type_code = types[type_idx];
        esc_ships[i].impact_power = impact_powers[type_idx];
        
        esc_ships[i].pos.x = ((double)rand() / RAND_MAX) * canvas_d;
        esc_ships[i].pos.y = ((double)rand() / RAND_MAX) * canvas_d;

        double range_angle = angle_ranges[type_idx];
        esc_ships[i].theta_L = ((double)rand() / RAND_MAX) * (90.0 - range_angle);
        esc_ships[i].theta_H = esc_ships[i].theta_L + range_angle;

        if (esc_ships[i].type_code == 'A') {
            esc_ships[i].v_max = 1.2 * v_max_b;
        } else {
            esc_ships[i].v_max = ((double)rand() / RAND_MAX) * v_max_b;
        }
        esc_ships[i].v_min = ((double)rand() / RAND_MAX) * esc_ships[i].v_max;

        esc_ships[i].r_min = calculate_range(esc_ships[i].v_min, esc_ships[i].theta_L);
        esc_ships[i].r_max = calculate_range(esc_ships[i].v_max, esc_ships[i].theta_H);
        esc_ships[i].is_destroyed = 0;
    }
}

void generate_path(PathPoint path[], int k, double canvas_d) {
    for (int i = 0; i < k; i++) {
        path[i].x = ((double)rand() / RAND_MAX) * canvas_d;
        path[i].y = ((double)rand() / RAND_MAX) * canvas_d;
    }
}

void save_initial_conditions(Battleship b, EscortShip esc_ships[], int count) {
    FILE *file = fopen("initial_battlefield.txt", "w");
    if (!file) return;

    fprintf(file, "--- INITIAL BATTLEFIELD CONDITIONS ---\n");
    fprintf(file, "Battleship Type: %c | Position: (%.2f, %.2f) | V_max: %.2f | Range: %.2f\n\n", 
            b.type_code, b.pos.x, b.pos.y, b.v_max, b.r_max);

    fprintf(file, "ID\tType\tPosition\t\tV_min\tV_max\tTheta_L\tTheta_H\tR_min\t\tR_max\n");
    for (int i = 0; i < count; i++) {
        fprintf(file, "%d\tE_%c\t(%.2f, %.2f)\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t\t%.2f\n",
                esc_ships[i].id, esc_ships[i].type_code, esc_ships[i].pos.x, esc_ships[i].pos.y,
                esc_ships[i].v_min, esc_ships[i].v_max, esc_ships[i].theta_L, esc_ships[i].theta_H,
                esc_ships[i].r_min, esc_ships[i].r_max);
    }
    fclose(file);
}

// ==========================================
// PART 1-A SIMULATION
// ==========================================
void run_part_1a_simulation(Battleship b, EscortShip esc_ships[], int count) {
    int sinking_escort_id = -1;

    for (int i = 0; i < count; i++) {
        double dist = get_distance(b.pos, esc_ships[i].pos);
        if (dist >= esc_ships[i].r_min && dist <= esc_ships[i].r_max) {
            sinking_escort_id = esc_ships[i].id;
            break;
        }
    }

    FILE *final_file = fopen("final_battlefield_part1a.txt", "w");

    if (sinking_escort_id != -1) {
        printf("\n====================================\n");
        printf("PART 1-A RESULT: Battleship SANK!\n");
        printf("Destroyed by Escort Ship Index: %d\n", sinking_escort_id);
        printf("====================================\n");

        if (final_file) {
            fprintf(final_file, "OUTCOME: Battleship SANK\n");
            fprintf(final_file, "Destroyed by Escort Ship ID: %d\n", sinking_escort_id);
        }
    } else {
        int hits = 0;
        printf("\n====================================\n");
        printf("PART 1-A RESULT: Battleship SURVIVED!\n");
        printf("Escort ships hit by Battleship:\n");

        if (final_file) {
            fprintf(final_file, "OUTCOME: Battleship SURVIVED\n");
            fprintf(final_file, "Index\tTime_to_Hit\tdistance\n");
        }

        for (int i = 0; i < count; i++) {
            double dist = get_distance(b.pos, esc_ships[i].pos);
            if (dist <= b.r_max) {
                esc_ships[i].is_destroyed = 1;
                hits++;
                printf(" -> Escort Ship ID %d (Type E_%c) hit at distance %.2f m\n", 
                       esc_ships[i].id, esc_ships[i].type_code, dist);
                
                if (final_file) {
                    fprintf(final_file, "%d\t0.0s\t\t%.2f\n", esc_ships[i].id, dist);
                }
            }
        }
        printf("Total Escort Ships Destroyed: %d / %d\n", hits, count);
        printf("====================================\n");
    }

    if (final_file) fclose(final_file);
}

// ==========================================
// PART 1-B SIMULATION
// ==========================================
void run_part_1b_simulations(Battleship b, EscortShip initial_escs[], int num_escorts, double canvas_d) {
    int k, t;
    double theta_min;

    printf("\n====================================\n");
    printf("         PART 1-B SIMULATION        \n");
    printf("====================================\n");
    printf("Enter number of path points (k): ");
    scanf("%d", &k);

    printf("Enter jam iteration threshold t (t < k): ");
    scanf("%d", &t);

    printf("Enter jammed min angle theta_min (0 < theta_min < 30): ");
    scanf("%lf", &theta_min);

    PathPoint path[k];
    generate_path(path, k, canvas_d);

    // --- SIMULATION 1: Normal Path Traversal ---
    printf("\n--- Running Part 1-B: Simulation 1 (Normal Path) ---\n");
    EscortShip escs_sim1[num_escorts];
    copy_escort_ships(initial_escs, escs_sim1, num_escorts);

    FILE *f1 = fopen("simulation1_results.txt", "w");
    fprintf(f1, "=== PART 1-B: SIMULATION 1 RESULTS ===\n");

    int b_sunk_sim1 = 0;
    for (int step = 0; step < k; step++) {
        b.pos.x = path[step].x;
        b.pos.y = path[step].y;
        
        fprintf(f1, "\nStep %d | Battleship Pos: (%.2f, %.2f)\n", step + 1, b.pos.x, b.pos.y);

        for (int i = 0; i < num_escorts; i++) {
            if (escs_sim1[i].is_destroyed) continue;

            double dist = get_distance(b.pos, escs_sim1[i].pos);
            if (dist >= escs_sim1[i].r_min && dist <= escs_sim1[i].r_max) {
                printf("Step %d: Battleship SANK at (%.2f, %.2f) by Escort ID %d!\n", 
                       step + 1, b.pos.x, b.pos.y, escs_sim1[i].id);
                fprintf(f1, "OUTCOME: Battleship SANK by Escort ID %d\n", escs_sim1[i].id);
                b_sunk_sim1 = 1;
                break;
            }
        }

        if (b_sunk_sim1) break;

        for (int i = 0; i < num_escorts; i++) {
            if (escs_sim1[i].is_destroyed) continue;

            double dist = get_distance(b.pos, escs_sim1[i].pos);
            if (dist <= b.r_max) {
                escs_sim1[i].is_destroyed = 1;
                printf("Step %d: Escort ID %d destroyed!\n", step + 1, escs_sim1[i].id);
                fprintf(f1, " -> Destroyed Escort ID %d at dist %.2f\n", escs_sim1[i].id, dist);
            }
        }
    }
    if (!b_sunk_sim1) {
        printf("Simulation 1 Completed: Battleship survived all %d path steps.\n", k);
        fprintf(f1, "OUTCOME: Battleship SURVIVED all steps.\n");
    }
    fclose(f1);

    // --- SIMULATION 2: Gun Jamming Path Traversal ---
    printf("\n--- Running Part 1-B: Simulation 2 (Gun Jamming) ---\n");
    EscortShip escs_sim2[num_escorts];
    copy_escort_ships(initial_escs, escs_sim2, num_escorts);

    FILE *f2 = fopen("simulation2_results.txt", "w");
    fprintf(f2, "=== PART 1-B: SIMULATION 2 RESULTS (Jammed after step %d) ===\n", t);

    int b_sunk_sim2 = 0;
    for (int step = 0; step < k; step++) {
        b.pos.x = path[step].x;
        b.pos.y = path[step].y;

        double current_r_min = 0.0;
        if (step >= t) {
            current_r_min = calculate_range(b.v_max, theta_min);
        }

        fprintf(f2, "\nStep %d | Pos: (%.2f, %.2f) | Range: [%.2f, %.2f]\n", 
                step + 1, b.pos.x, b.pos.y, current_r_min, b.r_max);

        for (int i = 0; i < num_escorts; i++) {
            if (escs_sim2[i].is_destroyed) continue;

            double dist = get_distance(b.pos, escs_sim2[i].pos);
            if (dist >= escs_sim2[i].r_min && dist <= escs_sim2[i].r_max) {
                printf("Step %d: Battleship SANK at (%.2f, %.2f) by Escort ID %d!\n", 
                       step + 1, b.pos.x, b.pos.y, escs_sim2[i].id);
                fprintf(f2, "OUTCOME: Battleship SANK by Escort ID %d\n", escs_sim2[i].id);
                b_sunk_sim2 = 1;
                break;
            }
        }

        if (b_sunk_sim2) break;

        for (int i = 0; i < num_escorts; i++) {
            if (escs_sim2[i].is_destroyed) continue;

            double dist = get_distance(b.pos, escs_sim2[i].pos);
            if (dist >= current_r_min && dist <= b.r_max) {
                escs_sim2[i].is_destroyed = 1;
                printf("Step %d: Escort ID %d destroyed!\n", step + 1, escs_sim2[i].id);
                fprintf(f2, " -> Destroyed Escort ID %d at dist %.2f\n", escs_sim2[i].id, dist);
            }
        }
    }
    if (!b_sunk_sim2) {
        printf("Simulation 2 Completed: Battleship survived all %d path steps.\n", k);
        fprintf(f2, "OUTCOME: Battleship SURVIVED all steps.\n");
    }
    fclose(f2);
}

// ==========================================
// PART 1-C SIMULATIONS (Cumulative Damage)
// ==========================================
void run_part_1c_simulation_A(Battleship b, EscortShip esc_ships[], int count) {
    double cumulative_damage = 0.0;
    int b_sunk = 0;

    // Escort ships attack B first
    for (int i = 0; i < count; i++) {
        double dist = get_distance(b.pos, esc_ships[i].pos);
        if (dist >= esc_ships[i].r_min && dist <= esc_ships[i].r_max) {
            cumulative_damage += esc_ships[i].impact_power;
            if (cumulative_damage >= 1.0) {
                b_sunk = 1;
                break;
            }
        }
    }

    FILE *f_out = fopen("part1c_static_results.txt", "w");
    fprintf(f_out, "=== PART 1-C (STATIC) RESULTS ===\n");

    printf("\n--- Part 1-C: Static Simulation ---\n");
    if (b_sunk) {
        printf("RESULT: Battleship SANK! Cumulative Damage reached 100%%\n");
        fprintf(f_out, "OUTCOME: Battleship SANK\nFinal Damage: 100%%\n");
    } else {
        printf("RESULT: Battleship SURVIVED! Total Cumulative Impact: %.2f%%\n", cumulative_damage * 100);
        fprintf(f_out, "OUTCOME: Battleship SURVIVED\nCumulative Damage: %.2f%%\n", cumulative_damage * 100);

        int hits = 0;
        for (int i = 0; i < count; i++) {
            double dist = get_distance(b.pos, esc_ships[i].pos);
            if (dist <= b.r_max) {
                esc_ships[i].is_destroyed = 1;
                hits++;
                fprintf(f_out, "Destroyed Escort ID %d at distance %.2f\n", esc_ships[i].id, dist);
            }
        }
        printf("Escort Ships Destroyed by Battleship: %d / %d\n", hits, count);
    }
    fclose(f_out);
}

void run_part_1c_simulation_B(Battleship b, EscortShip initial_escs[], int num_escorts, double canvas_d, int k, int t, double theta_min) {
    PathPoint path[k];
    generate_path(path, k, canvas_d);

    // --- SIMULATION 1: Normal Path with Cumulative Damage ---
    EscortShip escs_sim1[num_escorts];
    copy_escort_ships(initial_escs, escs_sim1, num_escorts);
    double b_damage = 0.0;
    int b_sunk = 0;

    FILE *f1 = fopen("part1c_sim1_results.txt", "w");
    fprintf(f1, "=== PART 1-C: SIMULATION 1 (Cumulative Damage) ===\n");

    for (int step = 0; step < k; step++) {
        b.pos.x = path[step].x;
        b.pos.y = path[step].y;

        for (int i = 0; i < num_escorts; i++) {
            if (escs_sim1[i].is_destroyed) continue;

            double dist = get_distance(b.pos, escs_sim1[i].pos);
            if (dist >= escs_sim1[i].r_min && dist <= escs_sim1[i].r_max) {
                b_damage += escs_sim1[i].impact_power;
                if (b_damage >= 1.0) {
                    b_sunk = 1;
                    fprintf(f1, "Step %d: Battleship SANK! Damage reached 100%%\n", step + 1);
    break;
                }
            }
        }

        if (b_sunk) break;

        for (int i = 0; i < num_escorts; i++) {
            if (escs_sim1[i].is_destroyed) continue;

            double dist = get_distance(b.pos, escs_sim1[i].pos);
           if (dist <= b.r_max) {
                escs_sim1[i].is_destroyed = 1;            }
        }
    }

    printf("\n--- Part 1-C: Path Simulation 1 Completed ---\n");
    if (b_sunk) {
        printf("Outcome: Battleship SANK during path traversal.\n");
    } else {
        printf("Outcome: Battleship SURVIVED! Final Cumulative Damage: %.2f%%\n", b_damage * 100);
        fprintf(f1, "OUTCOME: SURVIVED\nFinal Cumulative Damage: %.2f%%\n", b_damage * 100);
    }
    fclose(f1);
}


// ==========================================
// MAIN FUNCTION
// ==========================================
int main() {
    srand(time(NULL));

    double canvas_d;
    int num_escorts;
    char b_type;

    printf("====================================\n");
    printf("       ADVANCED NAVAL BATTLE        \n");
    printf("====================================\n\n");

    printf("Enter battlefield dimension: ");
    scanf("%lf", &canvas_d);

    printf("Enter number of Escort ships: ");
    scanf("%d", &num_escorts);

    printf("\nChoose Battleship type:\n");
    printf("U - USS Iowa\n");
    printf("M - MS King George V\n");
    printf("R - Richelieu\n");
    printf("S - Sovetsky Soyuz-class\n");
    printf("Enter choice: ");
    scanf(" %c", &b_type);

    Battleship b;
    b.type_code = b_type;
    b.pos.x = ((double)rand() / RAND_MAX) * canvas_d;
    b.pos.y = ((double)rand() / RAND_MAX) * canvas_d;
    b.v_max = 500.0; 
    b.r_max = calculate_range(b.v_max, 45.0);

    printf("\nBattleship created!\n");
    printf("Battleship type: %c\n", b.type_code);
    printf("Battleship position: (%.2f, %.2f)\n", b.pos.x, b.pos.y);

    EscortShip esc_ships[num_escorts];
    generate_escort_ships(esc_ships, num_escorts, canvas_d, b.v_max);

    save_initial_conditions(b, esc_ships, num_escorts);
    printf("\nInitial state saved to 'initial_battlefield.txt'.\n");

    // Execute Part 1-A
    run_part_1a_simulation(b, esc_ships, num_escorts);
    printf("Part 1-A state saved to 'final_battlefield_part1a.txt'.\n");

    // Execute Part 1-B
    run_part_1b_simulations(b, esc_ships, num_escorts, canvas_d);

    return 0;
}





