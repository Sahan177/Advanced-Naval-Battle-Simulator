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
    double gamma;
    int firing_count;
    double r_min, r_max;
    double reload_time;      // Added for Part 2
    double next_fire_time;   // Added for Part 2
    int is_destroyed;
} EscortShip;

typedef struct {
    char type_code;
    Position pos;
    double v_max;
    double r_max;
    double reload_time;
    double gamma;
    int firing_count;
    double impact_power;     // Added for Part 2
} Battleship;

typedef struct {
    double canvas_d;
    int num_escorts;
    unsigned int seed;

    Battleship b;
    EscortShip *esc_ships;

    int k;
    int t;
    double theta_min;

    int setup_complete;
} SimulationData;


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

double calculate_degraded_impact(double initial_impact,
                                 double gamma,
                                 int firing_count)
{
    return initial_impact *
           exp(-gamma * firing_count);
}


int can_hit_target(Position shooter, Position target,
                   double v_min, double v_max,
                   double theta_L, double theta_H,
                   double *firing_velocity, double *firing_angle)
{
    double distance = get_distance(shooter, target);

    /*
       Try angles throughout the allowed angle range.
       For each angle, calculate the velocity required
       to reach the target.
    */

    for (double angle = theta_L; angle <= theta_H; angle += 0.5)
    {
        double theta_rad = angle * (PI / 180.0);
        double sin_value = sin(2.0 * theta_rad);

        if (sin_value <= 0.0)
            continue;

        double required_velocity =
            sqrt((distance * GRAVITY) / sin_value);

        if (required_velocity >= v_min &&
            required_velocity <= v_max)
        {
            *firing_velocity = required_velocity;
            *firing_angle = angle;

            return 1;
        }
    }

    return 0;
}double calculate_flight_time(double distance,
                             double velocity,
                             double angle_deg)
{
    double angle_rad = angle_deg * (PI / 180.0);

    double horizontal_velocity =
        velocity * cos(angle_rad);

    if (horizontal_velocity <= 0.0)
        return -1.0;

    return distance / horizontal_velocity;
}

void calculate_range_bounds(EscortShip *ship) {
    double min_angle = ship->theta_L;
    double max_angle = ship->theta_H;

    double range_at_min_angle = calculate_range(ship->v_max, min_angle);
    double range_at_max_angle = calculate_range(ship->v_max, max_angle);
    double max_range = range_at_min_angle;

    if (range_at_max_angle > max_range) {
        max_range = range_at_max_angle;
    }

    if (min_angle <= 45.0 && max_angle >= 45.0) {
        max_range = calculate_range(ship->v_max, 45.0);
    }

    double min_range = calculate_range(ship->v_min, min_angle);
    double range_vmin_at_max_angle = calculate_range(ship->v_min, max_angle);

    if (range_vmin_at_max_angle < min_range) {
        min_range = range_vmin_at_max_angle;
    }

    if (min_angle <= 0.0 && max_angle >= 0.0) {
        min_range = 0.0;
    }

    if (min_angle <= 90.0 && max_angle >= 90.0) {
        min_range = 0.0;
    }

    ship->r_min = min_range;
    ship->r_max = max_range;
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
    double reload_times[5] = {3.0, 4.0, 5.0, 6.0, 7.0};
    double gamma_values[5] = {
    0.10,
    0.12,
    0.11,
    0.14,
    0.15
};

    for (int i = 0; i < count; i++) {
        esc_ships[i].id = i + 1;
        int type_idx = rand() % 5;
        esc_ships[i].type_code = types[type_idx];
        esc_ships[i].impact_power = impact_powers[type_idx];
        esc_ships[i].gamma = gamma_values[type_idx];
        esc_ships[i].firing_count = 0;
        esc_ships[i].reload_time = reload_times[type_idx];
        esc_ships[i].next_fire_time = 0.0;

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

        calculate_range_bounds(&esc_ships[i]);
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
    fprintf(file, "Battleship Type: %c | Position: (%.2f, %.2f) | V_max: %.2f | Range: %.2f | Reload: %.2fs\n\n", 
            b.type_code, b.pos.x, b.pos.y, b.v_max, b.r_max, b.reload_time);

    fprintf(file, "ID\tType\tPosition\t\tV_min\tV_max\tTheta_L\tTheta_H\tR_min\t\tR_max\tReload_T\n");
    for (int i = 0; i < count; i++) {
        fprintf(file, "%d\tE_%c\t(%.2f, %.2f)\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t\t%.2f\t%.2fs\n",
                esc_ships[i].id, esc_ships[i].type_code, esc_ships[i].pos.x, esc_ships[i].pos.y,
                esc_ships[i].v_min, esc_ships[i].v_max, esc_ships[i].theta_L, esc_ships[i].theta_H,
                esc_ships[i].r_min, esc_ships[i].r_max, esc_ships[i].reload_time);
    }
    fclose(file);
}

// ==========================================
// PART 1-A SIMULATION
// ==========================================

void run_part_1a_simulation(Battleship b,
                            EscortShip esc_ships[],
                            int count)
{
    int sinking_escort_id = -1;
    double sinking_time = 0.0;
    double sinking_velocity = 0.0;
    double sinking_angle = 0.0;

    /*
       Create a separate copy of the Escort fleet.
       This prevents Part 1-A from changing the
       original fleet used by later simulations.
    */
    EscortShip working_escorts[count];
copy_escort_ships(esc_ships, working_escorts, count);

    /*
       First check whether any Escort can hit
       the Battleship.
    */

    for (int i = 0; i < count; i++)
    {
        if (working_escorts[i].is_destroyed)
            continue;

        double firing_velocity;
        double firing_angle;

        int can_hit = can_hit_target(
            working_escorts[i].pos,
            b.pos,
            working_escorts[i].v_min,
            working_escorts[i].v_max,
            working_escorts[i].theta_L,
            working_escorts[i].theta_H,
            &firing_velocity,
            &firing_angle
        );

        if (can_hit)
        {
            double distance =
                get_distance(
                    working_escorts[i].pos,
                    b.pos
                );

            double flight_time =
                calculate_flight_time(
                    distance,
                    firing_velocity,
                    firing_angle
                );

            sinking_escort_id =
                working_escorts[i].id;

            sinking_time = flight_time;
            sinking_velocity = firing_velocity;
            sinking_angle = firing_angle;

            break;
        }
    }

    FILE *final_file =
        fopen("final_battlefield_part1a.txt", "w");

    /*
       Battleship was hit by an Escort.
    */
    if (sinking_escort_id != -1)
    {
        printf("\n====================================\n");
        printf("PART 1-A RESULT: Battleship SANK!\n");
        printf("Destroyed by Escort Ship ID: %d\n",
               sinking_escort_id);

        printf("Flight Time: %.2f seconds\n",
               sinking_time);

        printf("Firing Velocity: %.2f m/s\n",
               sinking_velocity);

        printf("Firing Angle: %.2f degrees\n",
               sinking_angle);

        printf("====================================\n");

        if (final_file)
        {
            fprintf(
                final_file,
                "OUTCOME: Battleship SANK\n"
            );

            fprintf(
                final_file,
                "Destroyed by Escort Ship ID: %d\n",
                sinking_escort_id
            );

            fprintf(
                final_file,
                "Flight Time: %.2f seconds\n",
                sinking_time
            );

            fprintf(
                final_file,
                "Firing Velocity: %.2f m/s\n",
                sinking_velocity
            );

            fprintf(
                final_file,
                "Firing Angle: %.2f degrees\n",
                sinking_angle
            );
        }
    }

    /*
       Battleship survived.
       Check which Escort ships can be hit.
    */
    else
    {
        int hits = 0;

        printf("\n====================================\n");
        printf("PART 1-A RESULT: Battleship SURVIVED!\n");
        printf("Escort ships hit by Battleship:\n");

        if (final_file)
        {
            fprintf(
                final_file,
                "OUTCOME: Battleship SURVIVED\n"
            );

            fprintf(
                final_file,
                "ID\tTime\tDistance\tVelocity\tAngle\n"
            );
        }

        for (int i = 0; i < count; i++)
        {
            if (working_escorts[i].is_destroyed)
                continue;

            double firing_velocity;
            double firing_angle;

            int can_hit = can_hit_target(
                b.pos,
                working_escorts[i].pos,
                0.0,
                b.v_max,
                0.0,
                90.0,
                &firing_velocity,
                &firing_angle
            );

            if (can_hit)
            {
                double distance =
                    get_distance(
                        b.pos,
                        working_escorts[i].pos
                    );

                double flight_time =
                    calculate_flight_time(
                        distance,
                        firing_velocity,
                        firing_angle
                    );

                working_escorts[i].is_destroyed = 1;

                hits++;

                printf(
                    " -> Escort ID %d (E_%c) hit | "
                    "Distance %.2f m | "
                    "Time %.2f s\n",
                    working_escorts[i].id,
                    working_escorts[i].type_code,
                    distance,
                    flight_time
                );

                if (final_file)
                {
                    fprintf(
                        final_file,
                        "%d\t%.2f\t%.2f\t%.2f\t%.2f\n",
                        working_escorts[i].id,
                        flight_time,
                        distance,
                        firing_velocity,
                        firing_angle
                    );
                }
            }
        }

        printf(
            "Total Escort Ships Destroyed: %d / %d\n",
            hits,
            count
        );

        printf("====================================\n");
    }

    if (final_file)
        fclose(final_file);
}

// ==========================================
// PART 1-B SIMULATION
// ==========================================
void run_part_1b_simulations(Battleship b, EscortShip initial_escs[], int num_escorts, double canvas_d, int *k_out, int *t_out, double *theta_min_out, PathPoint **path_out) {
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

    *k_out = k;
    *t_out = t;
    *theta_min_out = theta_min;

    PathPoint *path = malloc(k * sizeof(PathPoint));

if (path == NULL)
{
    printf("Memory allocation failed for path.\n");
    return;
}

generate_path(path, k, canvas_d);
*path_out = path;

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

            double firing_velocity;
            double firing_angle;

            int can_hit = can_hit_target(
             escs_sim1[i].pos,
             b.pos,
             escs_sim1[i].v_min,
             escs_sim1[i].v_max,
             escs_sim1[i].theta_L,
             escs_sim1[i].theta_H,
             &firing_velocity,
             &firing_angle
           );

            if (can_hit) {
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

            double firing_velocity;
            double firing_angle;

            int can_hit = can_hit_target(
             b.pos,
            escs_sim1[i].pos,
            0.0,
            b.v_max,
            0.0,
            90.0,
            &firing_velocity,
            &firing_angle
            );

            if (can_hit){
                escs_sim1[i].is_destroyed = 1;
                printf("Step %d: Escort ID %d destroyed!\n", step + 1, escs_sim1[i].id);
                fprintf(f1, " -> Destroyed Escort ID %d at dist %.2f\n", escs_sim1[i].id, 
                get_distance(b.pos, escs_sim1[i].pos));
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
    

        
        double current_r_max = b.r_max;
        double minimum_angle = 0.0;

        if (step >= t) {
         minimum_angle = theta_min;
        
    }

fprintf(f2, "\nStep %d | Pos: (%.2f, %.2f) | Minimum Angle: %.2f\n",
        step + 1, b.pos.x, b.pos.y, minimum_angle);

        for (int i = 0; i < num_escorts; i++) {
            if (escs_sim2[i].is_destroyed) continue;

            double firing_velocity;
            double firing_angle;

            int can_hit = can_hit_target(
    escs_sim2[i].pos,
    b.pos,
    escs_sim2[i].v_min,
    escs_sim2[i].v_max,
    escs_sim2[i].theta_L,
    escs_sim2[i].theta_H,
    &firing_velocity,
    &firing_angle
    );

if (can_hit) {
    printf("Step %d: Battleship SANK at (%.2f, %.2f) by Escort ID %d!\n",
           step + 1, b.pos.x, b.pos.y, escs_sim2[i].id);

    fprintf(f2,
            "OUTCOME: Battleship SANK by Escort ID %d | "
            "Velocity: %.2f | Angle: %.2f\n",
            escs_sim2[i].id,
            firing_velocity,
            firing_angle);

    b_sunk_sim2 = 1;
    break;
}
        }

        if (b_sunk_sim2) break;

        for (int i = 0; i < num_escorts; i++) {
            if (escs_sim2[i].is_destroyed) continue;

            double firing_velocity;
            double firing_angle;

            int can_hit = can_hit_target(
    b.pos,
    escs_sim2[i].pos,
    0.0,
    b.v_max,
    minimum_angle,
    90.0,
    &firing_velocity,
    &firing_angle
);

if (can_hit) {
    escs_sim2[i].is_destroyed = 1;

    printf("Step %d: Escort ID %d destroyed!\n",
           step + 1, escs_sim2[i].id);

    fprintf(f2,
            " -> Destroyed Escort ID %d | "
            "Distance: %.2f | Velocity: %.2f | Angle: %.2f\n",
            escs_sim2[i].id,
            get_distance(b.pos, escs_sim2[i].pos),
            firing_velocity,
            firing_angle);
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
void run_part_1c_simulation_A(Battleship b, EscortShip esc_ships[], int count)
{
    EscortShip working_escorts[count];
    copy_escort_ships(esc_ships, working_escorts, count);

    double cumulative_damage = 0.0;
    int b_sunk = 0;

    for (int i = 0; i < count; i++)
    {
        double firing_velocity;
        double firing_angle;

        int can_hit = can_hit_target(
            working_escorts[i].pos,
            b.pos,
            working_escorts[i].v_min,
            working_escorts[i].v_max,
            working_escorts[i].theta_L,
            working_escorts[i].theta_H,
            &firing_velocity,
            &firing_angle
        );

        if (can_hit)
        {
            cumulative_damage += working_escorts[i].impact_power;

            if (cumulative_damage >= 1.0)
            {
                b_sunk = 1;
                break;
            }
        }
    }

    FILE *f_out = fopen("part1c_static_results.txt", "w");

    if (f_out == NULL)
    {
        printf("Error: Could not create Part 1-C output file.\n");
        return;
    }

    fprintf(f_out, "=== PART 1-C (STATIC) RESULTS ===\n");

    printf("\n--- Part 1-C: Static Simulation ---\n");

    if (b_sunk)
    {
        printf("RESULT: Battleship SANK! Cumulative Damage reached 100%%\n");
        fprintf(f_out, "OUTCOME: Battleship SANK\n");
        fprintf(f_out, "Final Damage: 100%%\n");
    }
    else
    {
        printf(
            "RESULT: Battleship SURVIVED! "
            "Total Cumulative Impact: %.2f%%\n",
            cumulative_damage * 100
        );

        fprintf(
            f_out,
            "OUTCOME: Battleship SURVIVED\n"
            "Cumulative Damage: %.2f%%\n",
            cumulative_damage * 100
        );

        int hits = 0;

        for (int i = 0; i < count; i++)
        {
            double dist =
                get_distance(
                    b.pos,
                    working_escorts[i].pos
                );

            double firing_velocity;
            double firing_angle;

            int can_hit = can_hit_target(
                b.pos,
                working_escorts[i].pos,
                0.0,
                b.v_max,
                0.0,
                90.0,
                &firing_velocity,
                &firing_angle
            );

            if (can_hit)
            {
                working_escorts[i].is_destroyed = 1;
                hits++;

                fprintf(
                    f_out,
                    "Destroyed Escort ID %d at distance %.2f\n",
                    working_escorts[i].id,
                    dist
                );
            }
        }

        printf(
            "Escort Ships Destroyed by Battleship: %d / %d\n",
            hits,
            count
        );
    }

    fclose(f_out);
}

void run_part_1c_simulation_B(
    Battleship b,
    EscortShip initial_escs[],
    int num_escorts,
    PathPoint path[],
    int k,
    int t,
    double theta_min)
{
    EscortShip escs_sim1[num_escorts];
    copy_escort_ships(initial_escs, escs_sim1, num_escorts);

    double b_damage = 0.0;
    int b_sunk = 0;

    FILE *f1 = fopen("part1c_sim1_results.txt", "w");

    if (f1 == NULL)
    {
        printf("Error: Could not create Part 1-C output file.\n");
        return;
    }

    fprintf(f1,
            "=== PART 1-C: PATH CUMULATIVE DAMAGE ===\n\n");

    for (int step = 0; step < k; step++)
    {
        b.pos.x = path[step].x;
        b.pos.y = path[step].y;

        fprintf(f1,
                "Step %d | Battleship Position: (%.2f, %.2f)\n",
                step + 1,
                b.pos.x,
                b.pos.y);

        /* Escorts fire at the Battleship */
        for (int i = 0; i < num_escorts; i++)
        {
            if (escs_sim1[i].is_destroyed)
                continue;

            double firing_velocity;
            double firing_angle;

            int can_hit = can_hit_target(
                escs_sim1[i].pos,
                b.pos,
                escs_sim1[i].v_min,
                escs_sim1[i].v_max,
                escs_sim1[i].theta_L,
                escs_sim1[i].theta_H,
                &firing_velocity,
                &firing_angle
            );

            if (can_hit)
            {
                double damage =
                    escs_sim1[i].impact_power;

                b_damage += damage;

                fprintf(
                    f1,
                    " -> Escort ID %d hit Battleship | "
                    "Impact: %.2f%% | "
                    "Cumulative Damage: %.2f%%\n",
                    escs_sim1[i].id,
                    damage * 100,
                    b_damage * 100
                );

                if (b_damage >= 1.0)
                {
                    b_damage = 1.0;
                    b_sunk = 1;

                    fprintf(
                        f1,
                        "OUTCOME: Battleship SANK at Step %d\n",
                        step + 1
                    );

                    break;
                }
            }
        }

        if (b_sunk)
            break;

        /* Battleship attacks remaining Escorts */
        for (int i = 0; i < num_escorts; i++)
        {
            if (escs_sim1[i].is_destroyed)
                continue;

            double firing_velocity;
            double firing_angle;

            int can_hit = can_hit_target(
                b.pos,
                escs_sim1[i].pos,
                0.0,
                b.v_max,
                0.0,
                90.0,
                &firing_velocity,
                &firing_angle
            );

            if (can_hit)
            {
                escs_sim1[i].is_destroyed = 1;

                fprintf(
                    f1,
                    " -> Battleship destroyed Escort ID %d\n",
                    escs_sim1[i].id
                );
            }
        }

        fprintf(f1, "\n");
    }

    printf("\n--- Part 1-C: Path Simulation Completed ---\n");

    if (b_sunk)
    {
        printf(
            "Outcome: Battleship SANK during path traversal.\n"
        );

        printf(
            "Final Cumulative Damage: %.2f%%\n",
            b_damage * 100
        );

        fprintf(
            f1,
            "FINAL CUMULATIVE DAMAGE: %.2f%%\n",
            b_damage * 100
        );
    }
    else
    {
        printf(
            "Outcome: Battleship SURVIVED! "
            "Final Cumulative Damage: %.2f%%\n",
            b_damage * 100
        );

        fprintf(
            f1,
            "OUTCOME: Battleship SURVIVED\n"
            "Final Cumulative Damage: %.2f%%\n",
            b_damage * 100
        );
    }

    fclose(f1);
}

// ==========================================
// PART 2-A & 2-B SIMULATIONS (Cooldowns & Strategy)
// ==========================================
void run_part_2a_simulation(Battleship b, EscortShip initial_escs[], int count) {
    EscortShip escs[count];
    copy_escort_ships(initial_escs, escs, count);

    FILE *f_out = fopen("part2a_results.txt", "w");
    fprintf(f_out, "=== PART 2-A: BATTLESHIP COOLDOWN & STRATEGY ===\n");
    fprintf(f_out, "Battleship Firing Cooldown (T_B): %.2f seconds\n\n", b.reload_time);

    printf("\n====================================\n");
    printf("        PART 2-A SIMULATION         \n");
    printf("====================================\n");

    int in_range_indices[count];
    int target_count = 0;

    for (int i = 0; i < count; i++) {
        double firing_velocity;
        double firing_angle;

        int can_hit = can_hit_target(
    b.pos,
    escs[i].pos,
    0.0,
    b.v_max,
    0.0,
    90.0,
    &firing_velocity,
    &firing_angle
);

if (can_hit) {
    in_range_indices[target_count++] = i;
}
    }

    for (int i = 0; i < target_count - 1; i++) {
        for (int j = i + 1; j < target_count; j++) {
            int idx1 = in_range_indices[i];
            int idx2 = in_range_indices[j];
            if (escs[idx2].impact_power > escs[idx1].impact_power) {
                int temp = in_range_indices[i];
                in_range_indices[i] = in_range_indices[j];
                in_range_indices[j] = temp;
            }
        }
    }

    printf("Calculated Attack Strategy (%d targets in range):\n", target_count);
    fprintf(f_out, "Calculated Target Order:\n");

    double time_passed = 0.0;
    double cumulative_b_damage = 0.0;

for (int i = 0; i < target_count; i++)
{
    int idx = in_range_indices[i];

    double firing_velocity;
    double firing_angle;

    int can_hit = can_hit_target(
        escs[idx].pos, b.pos,
        escs[idx].v_min, escs[idx].v_max,
        escs[idx].theta_L, escs[idx].theta_H,
        &firing_velocity, &firing_angle
    );

    if (can_hit)
    {
        cumulative_b_damage += escs[idx].impact_power;
    }
}

    for (int i = 0; i < target_count; i++) {
        int idx = in_range_indices[i];
        escs[idx].is_destroyed = 1;
        printf(" -> Time %.2fs: Fired on Escort ID %d (Type E_%c, Threat Impact: %.2f)\n",
               time_passed, escs[idx].id, escs[idx].type_code, escs[idx].impact_power);
        fprintf(f_out, "Order %d | Time: %.2fs | Destroyed Escort ID %d (Type E_%c)\n",
                i + 1, time_passed, escs[idx].id, escs[idx].type_code);

        time_passed += b.reload_time;
    }

    printf("Total Execution Time: %.2f seconds\n", time_passed);
    printf("Battleship Cumulative Damage Taken: %.2f%%\n", cumulative_b_damage * 100);

    fprintf(f_out, "\nTotal Engagement Duration: %.2fs\n", time_passed);
    fprintf(f_out, "Battleship Cumulative Damage: %.2f%%\n", cumulative_b_damage * 100);
    fclose(f_out);
}

void run_part_2b_simulation(Battleship b, EscortShip initial_escs[], int count) {
    EscortShip escs[count];
    copy_escort_ships(initial_escs, escs, count);

    FILE *f_out = fopen("part2b_results.txt", "w");
    fprintf(f_out, "=== PART 2-B: ESCORT CONTINUOUS FIRING SIMULATION ===\n");

    printf("\n====================================\n");
    printf("        PART 2-B SIMULATION         \n");
    printf("====================================\n");

    double time_step = 0.5;
    double current_time = 0.0;
    double max_sim_time = 120.0;
    double b_damage = 0.0;
    double b_next_fire = 0.0;
    int b_sunk = 0;

    while (current_time <= max_sim_time && !b_sunk) {
        for (int i = 0; i < count; i++) {
            if (escs[i].is_destroyed) continue;

            double firing_velocity;
            double firing_angle;

            int can_hit = can_hit_target(
    escs[i].pos,
    b.pos,
    escs[i].v_min,
    escs[i].v_max,
    escs[i].theta_L,
    escs[i].theta_H,
    &firing_velocity,
    &firing_angle
    );

if (can_hit){
                if (current_time >= escs[i].next_fire_time) {
                    b_damage += escs[i].impact_power;
                    escs[i].next_fire_time = current_time + escs[i].reload_time;
                    fprintf(f_out, "[Time %.2fs] Escort ID %d FIRED! Damage: +%.2f%% (Total: %.2f%%)\n",
                            current_time, escs[i].id, escs[i].impact_power * 100, b_damage * 100);

                    if (b_damage >= 1.0) {
                        b_sunk = 1;
                        printf("Time %.2fs: Battleship SANK from continuous enemy fire!\n", current_time);
                        fprintf(f_out, "OUTCOME: Battleship SANK at Time %.2fs\n", current_time);
                        break;
                    }
                }
            }
        }

        if (b_sunk) break;

        if (current_time >= b_next_fire) {
            int best_target_idx = -1;
            double max_impact = -1.0;

            for (int i = 0; i < count; i++) {
                if (escs[i].is_destroyed) continue;
                double firing_velocity;
                double firing_angle;

                int can_hit = can_hit_target(
    b.pos,
    escs[i].pos,
    0.0,
    b.v_max,
    0.0,
    90.0,
    &firing_velocity,
    &firing_angle
);

if (can_hit)   { 
                    if (escs[i].impact_power > max_impact) {
                        max_impact = escs[i].impact_power;
                        best_target_idx = i;
                    }
                }
            }

            if (best_target_idx != -1) {
                escs[best_target_idx].is_destroyed = 1;
                b_next_fire = current_time + b.reload_time;
                printf("Time %.2fs: Battleship destroyed Escort ID %d (Type E_%c)\n",
                       current_time, escs[best_target_idx].id, escs[best_target_idx].type_code);
                fprintf(f_out, "[Time %.2fs] Battleship DESTROYED Escort ID %d\n",
                        current_time, escs[best_target_idx].id);
            }
        }

        current_time += time_step;
    }

    if (!b_sunk) {
        printf("Part 2-B Completed: Battleship survived with %.2f%% damage.\n", b_damage * 100);
        fprintf(f_out, "OUTCOME: Battleship SURVIVED | Final Damage: %.2f%%\n", b_damage * 100);
    }

    fclose(f_out);
}

void run_part_2c_simulation(Battleship b,
                            EscortShip initial_escs[],
                            int count)
{
    EscortShip escs[count];

    copy_escort_ships(initial_escs, escs, count);

    FILE *f_out = fopen("part2c_results.txt", "w");

    if (f_out == NULL)
    {
        printf("Error: Could not create Part 2-C output file.\n");
        return;
    }

    fprintf(f_out,
            "=== PART 2-C: IMPACT POWER DEGRADATION ===\n\n");

    printf("\n====================================\n");
    printf("        PART 2-C SIMULATION         \n");
    printf("====================================\n");

    double current_time = 0.0;

    for (int i = 0; i < count; i++)
    {
        if (escs[i].is_destroyed)
            continue;

        double firing_velocity;
        double firing_angle;

        int can_hit = can_hit_target(
            b.pos,
            escs[i].pos,
            0.0,
            b.v_max,
            0.0,
            90.0,
            &firing_velocity,
            &firing_angle
        );

        if (!can_hit)
        {
            printf(
                "\nEscort ID %d cannot be reached by Battleship.\n",
                escs[i].id
            );

            fprintf(
                f_out,
                "Escort ID %d cannot be reached by Battleship.\n\n",
                escs[i].id
            );

            continue;
        }

        double escort_damage = 0.0;
        int attack_number = 0;

        printf(
            "\nTarget Escort ID %d (Type E_%c)\n",
            escs[i].id,
            escs[i].type_code
        );

        fprintf(
            f_out,
            "Target Escort ID %d (Type E_%c)\n",
            escs[i].id,
            escs[i].type_code
        );

        while (escort_damage < 1.0 && attack_number < 100)
        {
            double current_impact =
                calculate_degraded_impact(
                    b.impact_power,
                    b.gamma,
                    attack_number
                );

            if (current_impact < 0.000001)
            {
                printf(
                    "Impact has become negligible after %d attacks.\n",
                    attack_number
                );

                fprintf(
                    f_out,
                    "Impact became negligible after %d attacks.\n",
                    attack_number
                );

                break;
            }

            attack_number++;

            escort_damage += current_impact;

            if (escort_damage > 1.0)
                escort_damage = 1.0;

            current_time += b.reload_time;

            printf(
                "Attack %d | Time %.2fs | "
                "Impact: %.2f%% | "
                "Gamma: %.3f | "
                "Cumulative Escort Damage: %.2f%%\n",
                attack_number,
                current_time,
                current_impact * 100,
                b.gamma,
                escort_damage * 100
            );

            fprintf(
                f_out,
                "Attack %d | Time %.2fs | "
                "Impact: %.2f%% | "
                "Gamma: %.3f | "
                "Cumulative Escort Damage: %.2f%%\n",
                attack_number,
                current_time,
                current_impact * 100,
                b.gamma,
                escort_damage * 100
            );

            if (escort_damage >= 1.0)
            {
                escs[i].is_destroyed = 1;

                printf(
                    "Escort ID %d DESTROYED after %d attacks.\n",
                    escs[i].id,
                    attack_number
                );

                fprintf(
                    f_out,
                    "Escort ID %d DESTROYED after %d attacks.\n\n",
                    escs[i].id,
                    attack_number
                );
            }
        }
    }

    printf(
    "\nRESULT: Battleship SURVIVED!\n"
    "Escort Ships Destroyed: %d / %d\n",
    total_destroyed,
    count
);

    fprintf(
    f_out,
    "\nOUTCOME: Battleship SURVIVED\n"
    "Escort Ships Destroyed: %d / %d\n",
    total_destroyed,
    count
);

    fclose(f_out);
}
void main_menu(SimulationData *sim);
void setup_menu(SimulationData *sim);
void start_simulation_menu(SimulationData *sim);
void instructions_menu(void);
void statistics_menu(void);
void battleship_properties(SimulationData *sim);
void escort_settings(SimulationData *sim);
void seed_settings(SimulationData *sim);
void initialize_simulation(SimulationData *sim);
void show_file(const char *filename);
void run_all_simulations(SimulationData *sim);

void main_menu(SimulationData *sim)
{
    int choice;
    char confirm;

    do
    {
        printf("\n");
        printf("====================================\n");
        printf("        ADVANCED NAVAL BATTLE       \n");
        printf("====================================\n");
        printf("1. Start Simulation\n");
        printf("2. View Instructions\n");
        printf("3. Simulation Statistics\n");
        printf("4. Exit\n");
        printf("====================================\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice)
        {
            case 1:
                start_simulation_menu(sim);
                break;

            case 2:
                instructions_menu();
                break;

            case 3:
                statistics_menu();
                break;

            case 4:
                printf("\nAre you sure you want to exit? (Y/N): ");
                scanf(" %c", &confirm);

                if (confirm == 'Y' || confirm == 'y')
                {
                    printf("\nExiting Advanced Naval Battle Simulator...\n");
                    return;
                }
                break;

            default:
                printf("\nInvalid choice. Please try again.\n");
        }

    } while (1);
}

void start_simulation_menu(SimulationData *sim)
{
    int choice;

    do
    {
        printf("\n");
        printf("====================================\n");
        printf("          START SIMULATION           \n");
        printf("====================================\n");
        printf("1. Setup Simulation\n");
        printf("2. Run All Simulations\n");
        printf("3. Run Part 1-A\n");
        printf("4. Run Part 1-B\n");
        printf("5. Run Part 1-C\n");
        printf("6. Run Part 2-A\n");
        printf("7. Run Part 2-B\n");
        printf("8. Run Part 2-C\n");
        printf("9. Return to Main Menu\n");
        printf("====================================\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice)
        {
            case 1:
                setup_menu(sim);
                break;

            case 2:
                if (!sim->setup_complete)
                {
                    printf("\nPlease complete the setup first.\n");
                }
                else
                {
                    run_all_simulations(sim);
                }
                break;

            case 3:
                if (!sim->setup_complete)
                {
                    printf("\nPlease complete the setup first.\n");
                }
                else
                {
                    run_part_1a_simulation(
                        sim->b,
                        sim->esc_ships,
                        sim->num_escorts
                    );
                }
                break;

            case 4:
                if (!sim->setup_complete)
                {
                    printf("\nPlease complete the setup first.\n");
                }
                else
                {
                    PathPoint *path = NULL;

                    run_part_1b_simulations(
                        sim->b,
                        sim->esc_ships,
                        sim->num_escorts,
                        sim->canvas_d,
                        &sim->k,
                        &sim->t,
                        &sim->theta_min,
                        &path
                    );

                    if (path != NULL)
                        free(path);
                }
                break;

            case 5:
                if (!sim->setup_complete)
                {
                    printf("\nPlease complete the setup first.\n");
                }
                else
                {
                    PathPoint *path = NULL;
                    int k;
                    int t;
                    double theta_min;

                    printf("\n====================================\n");
                    printf("        PART 1-C PATH SETUP         \n");
                    printf("====================================\n");

                    printf("Enter number of path points (k): ");
                    scanf("%d", &k);

                    printf("Enter jam iteration threshold t (t < k): ");
                    scanf("%d", &t);

                    printf("Enter jammed min angle theta_min (0 < theta_min < 30): ");
                    scanf("%lf", &theta_min);

                    path = malloc(k * sizeof(PathPoint));

                    if (path == NULL)
                    {
                        printf("Memory allocation failed for path.\n");
                        break;
                    }

                    generate_path(path, k, sim->canvas_d);

                    run_part_1c_simulation_A(
                        sim->b,
                        sim->esc_ships,
                        sim->num_escorts
                    );

                    run_part_1c_simulation_B(
                        sim->b,
                        sim->esc_ships,
                        sim->num_escorts,
                        path,
                        k,
                        t,
                        theta_min
                    );

                    free(path);
                }
                break;

            case 6:
                if (!sim->setup_complete)
                {
                    printf("\nPlease complete the setup first.\n");
                }
                else
                {
                    run_part_2a_simulation(
                        sim->b,
                        sim->esc_ships,
                        sim->num_escorts
                    );
                }
                break;

            case 7:
                if (!sim->setup_complete)
                {
                    printf("\nPlease complete the setup first.\n");
                }
                else
                {
                    run_part_2b_simulation(
                        sim->b,
                        sim->esc_ships,
                        sim->num_escorts
                    );
                }
                break;

            case 8:
                if (!sim->setup_complete)
                {
                    printf("\nPlease complete the setup first.\n");
                }
                else
                {
                    run_part_2c_simulation(
                        sim->b,
                        sim->esc_ships,
                        sim->num_escorts
                    );
                }
                break;

            case 9:
                return;

            default:
                printf("\nInvalid choice.\n");
        }

    } while (1);
}

void setup_menu(SimulationData *sim)
{
    int choice;

    do
    {
        printf("\n");
        printf("====================================\n");
        printf("           SIMULATION SETUP          \n");
        printf("====================================\n");
        printf("1. Battleship Properties\n");
        printf("2. Escort Ship Settings\n");
        printf("3. Random Seed\n");
        printf("4. Return\n");
        printf("====================================\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice)
        {
            case 1:
                battleship_properties(sim);
                break;

            case 2:
                escort_settings(sim);
                break;

            case 3:
                seed_settings(sim);
                break;

            case 4:
                return;

            default:
                printf("\nInvalid choice.\n");
        }

    } while (1);
}

void battleship_properties(SimulationData *sim)
{
    char type;

    printf("\n====================================\n");
    printf("       BATTLESHIP PROPERTIES        \n");
    printf("====================================\n");

    printf("Enter battlefield dimension: ");
    scanf("%lf", &sim->canvas_d);

    printf("Enter number of Escort ships: ");
    scanf("%d", &sim->num_escorts);

    printf("\nChoose Battleship type:\n");
    printf("U - USS Iowa\n");
    printf("M - MS King George V\n");
    printf("R - Richelieu\n");
    printf("S - Sovetsky Soyuz-class\n");
    printf("Enter choice: ");
    scanf(" %c", &type);

    if (type >= 'a' && type <= 'z')
        type = type - 'a' + 'A';

    sim->b.type_code = type;

    sim->b.pos.x =
        ((double)rand() / RAND_MAX) * sim->canvas_d;

    sim->b.pos.y =
        ((double)rand() / RAND_MAX) * sim->canvas_d;

    sim->b.v_max = 500.0;

    sim->b.r_max =
        calculate_range(sim->b.v_max, 45.0);

    sim->b.reload_time = 10.0;

    sim->b.gamma = 0.001;

    sim->b.firing_count = 0;

    sim->b.impact_power = 0.40;

    printf("\nBattleship configured successfully!\n");
    printf("Type: %c\n", sim->b.type_code);
    printf("Position: (%.2f, %.2f)\n",
           sim->b.pos.x,
           sim->b.pos.y);
    printf("Maximum velocity: %.2f m/s\n",
           sim->b.v_max);
    printf("Reload time: %.2f seconds\n",
           sim->b.reload_time);

          initialize_simulation(sim); 
}

void escort_settings(SimulationData *sim)
{
    int i;

    if (sim->esc_ships == NULL)
    {
        printf("\nEscort ships have not been generated yet.\n");
        printf("Configure Battleship Properties first.\n");
        return;
    }

    printf("\n====================================\n");
    printf("          ESCORT SHIP SETTINGS       \n");
    printf("====================================\n");

    for (i = 0; i < sim->num_escorts; i++)
    {
        printf("\nEscort ID: %d\n", sim->esc_ships[i].id);
        printf("Type: E%c\n", sim->esc_ships[i].type_code);
        printf("Velocity: %.2f - %.2f m/s\n",
               sim->esc_ships[i].v_min,
               sim->esc_ships[i].v_max);
        printf("Angle: %.2f - %.2f degrees\n",
               sim->esc_ships[i].theta_L,
               sim->esc_ships[i].theta_H);
        printf("Impact Power: %.2f\n",
               sim->esc_ships[i].impact_power);
        printf("Gamma: %.2f\n",
               sim->esc_ships[i].gamma);
        printf("Reload Time: %.2f seconds\n",
               sim->esc_ships[i].reload_time);
                   
    }

}

void seed_settings(SimulationData *sim)
{
    unsigned int seed;

    printf("\n====================================\n");
    printf("            RANDOM SEED              \n");
    printf("====================================\n");

    printf("Enter random seed: ");
    scanf("%u", &seed);

    sim->seed = seed;

    srand(sim->seed);

    printf("\nRandom seed updated to %u.\n", sim->seed);
}

void initialize_simulation(SimulationData *sim)
{
    if (sim->esc_ships != NULL)
    {
        free(sim->esc_ships);
        sim->esc_ships = NULL;
    }

    sim->esc_ships =
        malloc(sim->num_escorts * sizeof(EscortShip));

    if (sim->esc_ships == NULL)
    {
        printf("\nMemory allocation failed!\n");
        sim->setup_complete = 0;
        return;
    }

    generate_escort_ships(
        sim->esc_ships,
        sim->num_escorts,
        sim->canvas_d,
        sim->b.v_max
    );

    save_initial_conditions(
        sim->b,
        sim->esc_ships,
        sim->num_escorts
    );

    sim->k = 0;
    sim->t = 0;
    sim->theta_min = 0.0;

    sim->setup_complete = 1;

    printf("\n====================================\n");
    printf("       SIMULATION READY!             \n");
    printf("====================================\n");
    printf("Battlefield: %.2f x %.2f\n",
           sim->canvas_d,
           sim->canvas_d);
    printf("Escort ships: %d\n", sim->num_escorts);
    printf("Battleship: %c\n", sim->b.type_code);
    printf("Initial conditions saved.\n");
}

void run_all_simulations(SimulationData *sim)
{
    PathPoint *path = NULL;

    printf("\n====================================\n");
    printf("       RUNNING ALL SIMULATIONS       \n");
    printf("====================================\n");

    printf("\n--- PART 1-A ---\n");

    run_part_1a_simulation(
        sim->b,
        sim->esc_ships,
        sim->num_escorts
    );

    printf("\n--- PART 1-B ---\n");

    run_part_1b_simulations(
        sim->b,
        sim->esc_ships,
        sim->num_escorts,
        sim->canvas_d,
        &sim->k,
        &sim->t,
        &sim->theta_min,
        &path
    );

    printf("\n--- PART 1-C ---\n");

    run_part_1c_simulation_A(
        sim->b,
        sim->esc_ships,
        sim->num_escorts
    );

    run_part_1c_simulation_B(
        sim->b,
        sim->esc_ships,
        sim->num_escorts,
        path,
        sim->k,
        sim->t,
        sim->theta_min
    );

    printf("\n--- PART 2-A ---\n");

    run_part_2a_simulation(
        sim->b,
        sim->esc_ships,
        sim->num_escorts
    );

    printf("\n--- PART 2-B ---\n");

    run_part_2b_simulation(
        sim->b,
        sim->esc_ships,
        sim->num_escorts
    );

    printf("\n--- PART 2-C ---\n");

    run_part_2c_simulation(
        sim->b,
        sim->esc_ships,
        sim->num_escorts
    );

    if (path != NULL)
        free(path);

    printf("\n====================================\n");
    printf("       ALL SIMULATIONS COMPLETE      \n");
    printf("====================================\n");
}

void instructions_menu(void)
{
    printf("\n");
    printf("====================================\n");
    printf("            INSTRUCTIONS             \n");
    printf("====================================\n");

    printf("\nADVANCED NAVAL BATTLE SIMULATOR\n\n");

    printf("The simulator contains one Battleship and\n");
    printf("multiple stationary Axis Escort ships.\n");

    printf("\nMAIN OPTIONS:\n");
    printf("1. Start Simulation\n");
    printf("   Configure and execute the simulations.\n");

    printf("\n2. View Instructions\n");
    printf("   Display simulator instructions.\n");

    printf("\n3. Simulation Statistics\n");
    printf("   View saved simulation result files.\n");

    printf("\n4. Exit\n");
    printf("   Exit the simulator with confirmation.\n");

    printf("\nSIMULATION PARTS:\n");
    printf("Part 1-A : Basic attack simulation\n");
    printf("Part 1-B : Battleship movement simulation\n");
    printf("Part 1-C : Cumulative impact simulation\n");
    printf("Part 2-A : Battleship attack strategy\n");
    printf("Part 2-B : Continuous escort firing\n");
    printf("Part 2-C : Degrading impact power\n");

    printf("\nProjectile motion uses gravity = %.2f m/s^2.\n",
           GRAVITY);

    printf("\nPress ENTER to return to the menu...");
    getchar();
    getchar();
}

void show_file(const char *filename)
{
    FILE *file;
    char line[256];

    file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("\nFile '%s' does not exist yet.\n", filename);
        return;
    }

    printf("\n====================================\n");
    printf("FILE: %s\n", filename);
    printf("====================================\n");

    while (fgets(line, sizeof(line), file) != NULL)
    {
        printf("%s", line);
    }

    fclose(file);

    printf("\n====================================\n");
}

void statistics_menu(void)
{
    int choice;

    do
    {
        printf("\n");
        printf("====================================\n");
        printf("       SIMULATION STATISTICS         \n");
        printf("====================================\n");
        printf("1. Initial Battlefield\n");
        printf("2. Part 1-A Results\n");
        printf("3. Part 1-B Simulation 1\n");
        printf("4. Part 1-B Simulation 2\n");
        printf("5. Part 1-C Static Results\n");
        printf("6. Part 1-C Simulation Results\n");
        printf("7. Part 2-A Results\n");
        printf("8. Part 2-B Results\n");
        printf("9. Part 2-C Results\n");
        printf("10. Return\n");
        printf("====================================\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice)
        {
            case 1:
                show_file("initial_battlefield.txt");
                break;

            case 2:
                show_file("final_battlefield_part1a.txt");
                break;

            case 3:
                show_file("simulation1_results.txt");
                break;

            case 4:
                show_file("simulation2_results.txt");
                break;

            case 5:
                show_file("part1c_static_results.txt");
                break;

            case 6:
                show_file("part1c_sim1_results.txt");
                break;

            case 7:
                show_file("part2a_results.txt");
                break;

            case 8:
                show_file("part2b_results.txt");
                break;

            case 9:
                show_file("part2c_results.txt");
                break;

            case 10:
                return;

            default:
                printf("\nInvalid choice.\n");
        }

    } while (1);
}



// ==========================================
// MAIN FUNCTION
// ==========================================
int main(void)
{
    SimulationData sim;

    sim.canvas_d = 0.0;
    sim.num_escorts = 0;
    sim.seed = (unsigned int)time(NULL);

    sim.esc_ships = NULL;

    sim.k = 0;
    sim.t = 0;
    sim.theta_min = 0.0;

    sim.setup_complete = 0;

    srand(sim.seed);

    main_menu(&sim);

    if (sim.esc_ships != NULL)
    {
        free(sim.esc_ships);
    }

    return 0;
}
