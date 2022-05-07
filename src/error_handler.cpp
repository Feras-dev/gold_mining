/**
 * @file error_handler.cpp
 * @author Feras Alshehri (falshehri@mail.csuchico.edu)
 * @brief entry point of our gold chase game.
 * @version 0.1
 * @date 2022-03-01
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <iostream>
#include <stdio.h> // for perror

#include "error_handler.h"

/**
 * @brief prints a specific message to the user corresponding to the
 *          error_code given, invokes any necessary cleaning routines,
 *          and returns.
 *
 * @param error_code see error_code enum in error_handler.h
 */
void handle_error(ERROR_CODE_E error_code) {
    // @TODO convert cout to perror() calls ...
    switch (error_code) {
    case ok:
        perror("Success!");
        break;
    case error_map_file_specified_by_subsequent_player:
        perror("ERROR: subsequent player specified a map file");
        break;
    case error_no_map_file_specified_by_first_player:
        perror("ERROR: no map file is given by first player");
        break;
    case error_map_file_specified_is_not_valid:
        perror("ERROR: map file specified is not valid");
        break;
    case error_map_constructor_threw_an_exception:
        printf("ERROR: constructor of map class threw and exception");
        break;
    case error_in_shm_open:
        perror("ERROR: error in shem_open()");
        break;
    case error_in_shm_unlink:
        perror("ERROR: error in shem_unlink()");
        break;
    case error_in_sem_unlink:
        perror("ERROR: error in sem_unlink()");
        break;
    case error_in_sem_close:
        perror("ERROR: error in sem_close()");
        break;
    case error_in_sem_wait:
        perror("ERROR: error in sem_wait()");
        break;
    case error_in_sem_post:
        perror("ERROR: error in sem_post()");
        break;
    case error_failed_initialization:
        perror("ERROR: initialization failed");
        break;
    case error_failed_map_rendering:
        perror("ERROR: failed to render map");
        break;
    case error_in_ftruncate:
        perror("ERROR: error in ftruncate()");
        break;
    case error_in_mmap:
        perror("ERROR: error in mmap()");
        break;
    case error_illegal_charecter_in_map_file:
        perror("ERROR: detected an illegal charecter in map file (num gold, then only space, newline, and asterisk are legal)");
        break;
    case error_max_number_of_players_reached:
        printf("ERROR: maximum number of players reached! (max=5)");
        break;
    }
}