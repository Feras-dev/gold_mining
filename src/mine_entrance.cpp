/**
 * @file mine_entrance.cpp
 * @author Feras Alshehri (falshehri@mail.csuchico.edu)
 * @brief entry point of our gold chase game.
 * @version 0.1
 * @date 2022-03-01
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <algorithm>
#include <cstring>
#include <errno.h>
#include <fcntl.h> /* For O_* constants */
#include <iostream>
#include <random>
#include <semaphore.h>
#include <stdio.h> // for perror
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <unistd.h>

#include "Map.h"
#include "error_handler.h"
#include "goldchase.h"
#include "map_parser.h"
#include "mine_entrance.h"

#define SEMAPHORE_NAME "/goldchase_semaphore"
#define SHARED_MEM_NAME "/goldchase_shared_mem"
#define SYSCALL_OK 0
#define MAX_NUM_PLAYERS 5

#define DEBUG(x) (std::cout << x << "\n")

// semaphore and shared memory andother variables declared as a file global since it's
// used frequently
static sem_t       *semaphore = nullptr;
static int          shared_mem_fd;
static unsigned int player_number     = 0;
static bool         player_found_gold = false;
static goldMine_S  *gmp;

/**
 * @brief returns a random number between 0 and (x*y).
 *
 * @param x number of cols.
 * @param y number of rows.
 * @return unsigned int a random number.
 */
unsigned int get_random_number(int x, int y) {
    std::random_device                 rd;
    std::mt19937                       rng(rd());
    std::uniform_int_distribution<int> uni(0, (x * y) - 1);

    return uni(rng);
}

/**
 * @brief convert the player number from an int to the bit mask.
 *
 * @param pn player number (between [1..5] all inclusive).
 * @return unsigned char player bit mask.
 */
unsigned char pn_to_player_bit_mask(unsigned int pn) {
    switch (pn) {
    case 1:
        return G_PLR0;
    case 2:
        return G_PLR1;
    case 3:
        return G_PLR2;
    case 4:
        return G_PLR3;
    case 5:
        return G_PLR4;
    default:
        return 99; // invalid pn... need better containment here
    };
}

/**
 * @brief turn a player's bit on in the player's mask.
 *
 * @param pn player number
 * @return true
 * @return false
 */
void set_player_bit(unsigned int pn) { gmp->players |= pn_to_player_bit_mask(pn); }

/**
 * @brief turn a player's bit off in the player's mask.
 *
 * @param pn player number
 */
void reset_player_bit(unsigned int pn) { gmp->players &= ~pn_to_player_bit_mask(pn); }

/**
 * @brief shared memory clean up. need to make sure that semaphores are posted before
 * invoking this function.
 *
 */
void clean_up() {
    // remove player from map and reset their bit
    if ((player_number > 0) && (player_number < 6)) {
        reset_player_bit(player_number);
        for (unsigned int i = 0; i < (gmp->cols * gmp->rows); ++i) {
            if ((unsigned int)gmp->map[i] == pn_to_player_bit_mask(player_number)) {
                gmp->map[i] = 0;
                break;
            }
        }
    }

    // if this function was invoked by the only active player (last player in the
    // game), then clean shared memory and semaphore
    bool last_one_in_game = ((unsigned int)gmp->players == 0) ? true : false;
    if (last_one_in_game) {
        if (sem_close(semaphore) != SYSCALL_OK) { handle_error(error_in_sem_close); }
        if (sem_unlink(SEMAPHORE_NAME) != SYSCALL_OK) {
            handle_error(error_in_sem_unlink);
        }
        if (shm_unlink(SHARED_MEM_NAME) != SYSCALL_OK) {
            handle_error(error_in_shm_unlink);
        }
    }

    DEBUG(std::to_string(player_number));
}

void render_map(Map &goldMine) {

    std::string str = "player #";
    str += std::to_string(player_number);
    char cstr[str.length() + 1];
    std::strcpy(cstr, str.c_str());

    goldMine.postNotice(cstr);
}

/**
 * @brief non-blocking function. Could be used in a while loop to wait for semaphore.
 *
 * @return true if semaphore is available
 * @return false otherwise
 */
bool check_semaphore_availability() {
    int semval;
    sem_getvalue(semaphore, &semval);
    return semval > 0 ? true : false;
}

/**
 * @brief initialization routine to determine player type and number and set up game
 *
 * @param argc number of command line arguments
 */
void initialization_routine(int argc) {

    if (argc > 1) {
        // assume first player
        semaphore =
            sem_open(SEMAPHORE_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR, 1);

        if (semaphore != SEM_FAILED) {
            // successfully created semaphore, correct number of cmd args given, treat
            // user as a first player
            player_number = 1;
        } else {
            if (errno == EEXIST) {
                handle_error(error_map_file_specified_by_subsequent_player);
            } else {
                perror("Failed sem_open 1");
            }
            clean_up();
        }
    } else {
        // assume subsequent player
        semaphore = sem_open(SEMAPHORE_NAME, O_RDWR);

        if (semaphore != SEM_FAILED) {
            player_number = 2; // temporary for any subsequent player
        } else {
            if (errno == ENOENT) {
                handle_error(error_no_map_file_specified_by_first_player);
            } else {
                perror("Failed sem_open 2");
            }
            clean_up();
        }
    }

    return;
}

/**
 * @brief initialize first player process.
 *
 * @param map_file path to map text file, passed by first player as a command line
 * argument
 * @return true first player successful initialization
 * @return false otherwise
 */
bool run_first_player_init_routine(std::string map_file) {

    bool success = false;

    // take semaphore should still be available
    if (sem_wait(semaphore) != SYSCALL_OK) {
        handle_error(error_in_sem_wait);
        success = false;
    } else {
        // parse map
        Map_parser my_map(map_file);

        if (!my_map.is_good()) {
            handle_error(error_map_file_specified_is_not_valid);
            success = false;
        } else {
            // create shared mem
            shared_mem_fd =
                shm_open(SHARED_MEM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);

            if (shared_mem_fd < 0) {
                handle_error(error_in_shm_open);
                success = false;
            } else {
                // Set shared game size
                if (ftruncate(shared_mem_fd, sizeof(goldMine_S) +
                                                 my_map.get_cols() * my_map.get_rows()) ==
                    -1) {
                    handle_error(error_in_ftruncate);
                }

                // initialize map data
                gmp = (goldMine_S *)mmap(
                    nullptr, sizeof(goldMine_S) + my_map.get_cols() * my_map.get_rows(),
                    PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem_fd, 0);
                if (gmp == MAP_FAILED) {
                    handle_error(error_in_mmap);
                } else {
                    gmp->cols = my_map.get_cols();
                    gmp->rows = my_map.get_rows();

                    my_map.slurp_map(gmp);
                    if (!my_map.is_good()) { std::cout << "failed slurp\n"; }

                    success = true;
                }
            }
        }
    }

    // give semaphore
    if (sem_post(semaphore) != SYSCALL_OK) { handle_error(error_in_sem_post); }

    // given others a chance to grab the semaphore if they have been waiting for it
    // @TODO: make this usleep ... 1 sec is too long
    sleep(1);

    return success;
}

/**
 * @brief initialize subsequent player process.
 *
 * @return true subsequent player successful initialization
 * @return false otherwise
 */
bool run_subsequent_player_init_routine() {

    bool success = false;

    // twiddle thumbs until semaphore is available
    while (!check_semaphore_availability()) {}

    // take semaphore
    if (sem_wait(semaphore) != SYSCALL_OK) {
        handle_error(error_in_sem_wait);
        success = false;
    } else {
        // open shared mem
        shared_mem_fd = shm_open(SHARED_MEM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
        if (shared_mem_fd < 0) {
            handle_error(error_in_shm_open);
            success = false;
        } else { // successful shm_open call
            gmp = (goldMine_S *)mmap(nullptr, sizeof(goldMine_S), PROT_READ | PROT_WRITE,
                                     MAP_SHARED, shared_mem_fd, 0);
            if (gmp == MAP_FAILED) {
                handle_error(error_in_mmap);
            } else {
                // get actual player number (lowest available between 1 and 5, all
                // inclusive)
                if (pn_to_player_bit_mask(1) & ~gmp->players) {
                    player_number = 1;
                    success       = true;
                } 
                else if (pn_to_player_bit_mask(2) & ~gmp->players) {
                    player_number = 2;
                    success       = true;
                } 
                else if (pn_to_player_bit_mask(3) & ~gmp->players) {
                    player_number = 3;
                    success       = true;
                } 
                else if (pn_to_player_bit_mask(4) & ~gmp->players) {
                    player_number = 4;
                    success       = true;
                } 
                else if (pn_to_player_bit_mask(5) & ~gmp->players) {
                    player_number = 5;
                    success       = true;
                } 
                else {
                    handle_error(error_max_number_of_players_reached);
                    success = false;
                    // this will tell the clean up function to not look for this
                    // player on the map as it was never placed
                    player_number = 6;
                }
            }
        }
    }

    // give semaphore
    if (sem_post(semaphore) != SYSCALL_OK) { handle_error(error_in_sem_post); }

    // given others a chance to grab the semaphore if they have been waiting for it
    // @TODO: make this usleep ... 1 sec is too long
    sleep(1);

    return success;
}

/**
 * @brief move a player one bit in any direction in 2-D space if legal.
 *        legal move example: into an empty cell or gold.
 *        illegal move example: into a wall or past edge of map.
 *
 * @param player_bit_mask
 * @param target_location
 * @return true if move is allowed.
 * @return false move was illegal and should be ignored.
 */
void move_player(unsigned int player_bit_mask, unsigned int current_location,
                 unsigned int target_location, Map &goldMineM) {
    bool player_found_real_gold  = (gmp->map[target_location] == G_GOLD);
    bool player_found_fools_gold = (gmp->map[target_location] == G_FOOL);

    // check if player found gold
    if (player_found_real_gold) {
        goldMineM.postNotice("found real gold!");
        goldMineM.postNotice("You Won!");
        player_found_gold = true;
    }
    if (player_found_fools_gold) { goldMineM.postNotice("found fool's gold!"); }

    // move player to target location and reset it's previous location
    gmp->map[current_location] = 0; // empty
    gmp->map[target_location]  = player_bit_mask;
}

/**
 * @brief responds to input keys accordingly to enable player navigation.
 *
 * @param input input key recorded from player's keyboard.
 * @param goldMineM Map object
 */
bool controller(int input, Map &goldMineM) {
    unsigned int pn                              = 0; // player's number (1..5)
    unsigned int pl                              = 0; // player's current location
    unsigned int tl                              = 0; // player's target location
    bool         move_requested                  = false;
    bool         player_is_not_going_off_the_map = false;
    bool         exit_requested                  = false;

    // get player bit mask
    switch (player_number) {
    case 1:
        pn = G_PLR0;
        break;
    case 2:
        pn = G_PLR1;
        break;
    case 3:
        pn = G_PLR2;
        break;
    case 4:
        pn = G_PLR3;
        break;
    case 5:
        pn = G_PLR4;
        break;
    default:
        break;
    }

    // get player's location
    if (pn > 0) {
        for (unsigned int i = 0; i < (gmp->cols * gmp->rows); ++i) {
            if ((unsigned int)gmp->map[i] == pn) {
                pl = i;
                break;
            }
        }
    }

    // calculate target cell location based on input
    //     ^
    //     |
    // <-hjkl->
    //    |
    //    v
    switch (input) {

    case int('h'):
        // fall through
    case int('H'):
        // check player location against left edge of map
        player_is_not_going_off_the_map = ((pl % (gmp->cols)) > 0);
        if (player_is_not_going_off_the_map || player_found_gold) {
            tl = pl - 1;                                // move left
            if (gmp->map[tl] & (unsigned char)G_ANYP) { // go over player
                tl                              = tl - 1;
                player_is_not_going_off_the_map = ((pl % (gmp->cols)) > 0);
                if ((player_is_not_going_off_the_map || player_found_gold)) {
                    move_requested = true;
                }
            } else {
                move_requested = true;
            }
        }
        break;

    case int('j'):
        // fall through
    case int('J'):
        // check player location against left edge of map
        player_is_not_going_off_the_map = ((pl / (gmp->rows)) < (gmp->rows * 2 + 1));
        if (player_is_not_going_off_the_map || player_found_gold) {
            tl = pl + gmp->cols;                        // move down
            if (gmp->map[tl] & (unsigned char)G_ANYP) { // go over player
                tl = tl + gmp->cols;
                player_is_not_going_off_the_map =
                    ((tl / (gmp->rows)) < (gmp->rows * 2 + 1));
                if ((player_is_not_going_off_the_map || player_found_gold)) {
                    move_requested = true;
                }
            } else {
                move_requested = true;
            }
        }
        break;

    case int('k'):
        // fall through
    case int('K'):
        // check player location against top edge of map
        player_is_not_going_off_the_map = ((pl / (gmp->rows)) > 1);
        if (player_is_not_going_off_the_map || player_found_gold) {
            tl = std::max((int)pl - gmp->cols, 0);      // move up
            if (gmp->map[tl] & (unsigned char)G_ANYP) { // go over player
                tl                              = tl - gmp->cols;
                player_is_not_going_off_the_map = ((tl / (gmp->rows)) > 1);
                if ((player_is_not_going_off_the_map || player_found_gold)) {
                    move_requested = true;
                }
            } else {
                move_requested = true;
            }
        }
        break;

    case int('l'):
        // fall through
    case int('L'):
        // check player location against top edge of map
        player_is_not_going_off_the_map = ((pl % (gmp->cols)) < (gmp->cols - 1));
        if (player_is_not_going_off_the_map || player_found_gold) {
            tl = pl + 1;                                // move right
            if (gmp->map[tl] & (unsigned char)G_ANYP) { // go over player
                tl                              = tl + 1;
                player_is_not_going_off_the_map = ((tl % (gmp->cols)) < (gmp->cols - 1));
                if ((player_is_not_going_off_the_map || player_found_gold)) {
                    move_requested = true;
                }
            } else {
                move_requested = true;
            }
        }
        break;

    default:
        break;
    };

    // if move requested, move player and (expose gold if any), else ignore
    if (move_requested && (gmp->map[tl] != G_WALL)) {
        move_player(pn, pl, tl, goldMineM);
        move_requested = false;
    }

    // if we have gone over the edge, and have found gold, quit.
    if (player_found_gold && !player_is_not_going_off_the_map) { exit_requested = true; }

    return exit_requested;
}

/**
 * @brief main loop. to be invoked after proper initialization.
 *
 */
void main_loop() {
    bool exit_requested = false;

    // place current player randomly in empty spaces in map
    set_player_bit(player_number);
    while (1) {
        unsigned int r = get_random_number(gmp->rows, gmp->cols);
        if (gmp->map[r] == 0) {
            gmp->map[r] = pn_to_player_bit_mask(player_number);
            break;
        }
    }

    try {
        Map goldMineM(gmp->map, gmp->rows, gmp->cols);
        render_map(goldMineM);

        while (!exit_requested) {
            // update map
            goldMineM.drawMap();

            // get user input
            // H, J, K, or L to move. Q to quit.
            int input = goldMineM.getKey();

            switch (input) {

            case int('h'):
                // fall through
            case int('H'):
                // fall through
            case int('j'):
                // fall through
            case int('J'):
                // fall through
            case int('k'):
                // fall through
            case int('K'):
                // fall through
            case int('l'):
                // fall through
            case int('L'):
                // twiddle thumbs until semaphore is available
                while (!check_semaphore_availability()) {}

                // take semaphore
                if (sem_wait(semaphore) != SYSCALL_OK) {
                    handle_error(error_in_sem_wait);
                } else {
                    exit_requested = controller(input, goldMineM); // handle any move key
                    // give semaphore
                    if (sem_post(semaphore) != SYSCALL_OK) {
                        handle_error(error_in_sem_post);
                    }
                }
                break;

            case int('q'):
                // fall through
            case int('Q'):
                exit_requested = true;
                break;

            default:
                break;
            };
        }

    } catch (const std::exception &e) {
        handle_error(error_map_constructor_threw_an_exception);
        std::cerr << e.what() << '\n';
    }
}

int main(int argc, char *argv[]) {
    bool init_went_ok = false;

    // set player number
    initialization_routine(argc);

    // initialize game: varies based on first vs subsequent player
    if (player_number == 0) {
        exit(1);
    } else if (player_number == 1) {
        init_went_ok = run_first_player_init_routine((std::string)argv[1]);
    } else { // if ((player_number > 1)) {
        init_went_ok = run_subsequent_player_init_routine();
    }

    // main loop -- all players run here
    if (init_went_ok) {
        main_loop();
        clean_up();
    } else {
        handle_error(error_failed_initialization);
    }

    return 0;
}