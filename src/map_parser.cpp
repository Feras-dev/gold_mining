/**
 * @file map_parser.cpp
 * @author Feras Alshehri (falshehri@mail.csuchico.edu)
 * @brief class to parse the map from a text file into an object that can
 *          be consumed by our game.
 * @version 0.1
 * @date 2022-03-01
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <fstream>
#include <iostream>
#include <random>

#include "error_handler.h"
#include "goldchase.h"
#include "map_parser.h"

Map_parser::Map_parser(std::string path_to_map_file) {
    std::string l;
    char        map_legal_chars[]        = {' ', '\n', '*'};
    char        first_line_legal_chars[] = {' ', '\n', '1', '2', '3', '4',
                                     '5', '6',  '7', '8', '9', '0'};

    // open file and set class attributes
    std::ifstream fs(path_to_map_file);

    if (fs.is_open()) {
        // parse first line (gold count)
        getline(fs, l);
        // check for illegal charecters
        for (auto &c : l) {
            if ((c != first_line_legal_chars[0]) && (c != first_line_legal_chars[1]) &&
                (c != first_line_legal_chars[2]) && (c != first_line_legal_chars[3]) &&
                (c != first_line_legal_chars[4]) && (c != first_line_legal_chars[5]) &&
                (c != first_line_legal_chars[6]) && (c != first_line_legal_chars[7]) &&
                (c != first_line_legal_chars[8]) && (c != first_line_legal_chars[9]) &&
                (c != first_line_legal_chars[10]) && (c != first_line_legal_chars[11])) {
                handle_error(error_illegal_charecter_in_map_file);
                is_good_ = false;
                return;
            }
        }

        total_gold_count = std::stoul(l);
        fools_gold_count = total_gold_count - REAL_GOLD_COUNT;

        // parse rest of the map file (actual map)
        while (std::getline(fs, l)) {
            // check for illegal charecters
            for (auto &c : l) {
                // @TODO: ugly! you can find a cleaner way to do this :/
                if ((c != map_legal_chars[0]) && (c != map_legal_chars[1]) &&
                    (c != map_legal_chars[2])) {
                    handle_error(error_illegal_charecter_in_map_file);
                    is_good_ = false;
                    return;
                }
            }
            // update rows and columns as we scan the textfile
            if (l.length() > columns) { columns = l.length(); }
            rows++;
        }
        // close file
        fs.close();
        is_good_      = true;
        map_file_path = path_to_map_file;
    } else {
        is_good_ = false;
    }
}

Map_parser::~Map_parser() {}
bool         Map_parser::is_good() { return is_good_; }
unsigned int Map_parser::get_rows() { return rows; }
unsigned int Map_parser::get_cols() { return columns; }
unsigned int Map_parser::get_count_of_total_gold() { return total_gold_count; }
unsigned int Map_parser::get_count_of_fools_gold() { return fools_gold_count; }

unsigned int Map_parser::get_random_number() {
    std::random_device                 rd;
    std::mt19937                       rng(rd());
    std::uniform_int_distribution<int> uni(0, (rows * columns) - 1);

    return uni(rng);
}

void Map_parser::slurp_map(goldMine_S *gmp) {
    std::string l;
    int         current_position_abs;

    is_good_ = false;

    // open file and set class attributes
    std::ifstream fs(map_file_path);

    if (fs.is_open()) {
        // ignore first line (gold count)
        getline(fs, l);

        // parse the map
        for (int cur_row = 0; cur_row < rows; ++cur_row) {
            getline(fs, l);
            for (int cur_col = 0; cur_col < columns; ++cur_col) {
                current_position_abs = cur_row * columns + cur_col;
                if (l[cur_col] == ' ') {
                    gmp->map[current_position_abs] = 0;
                } else if (l[cur_col] == '*') {
                    gmp->map[current_position_abs] = G_WALL;
                }
            }
        }
        // close file
        fs.close();

        if (total_gold_count > 0) {

            // place real gold randomly in empty spaces in map
            while (1) {
                unsigned int r = get_random_number();
                if (gmp->map[r] == 0) {
                    gmp->map[r] = G_GOLD;
                    break;
                }
            }

            // place fool's gold randomly in empty spaces in map
            int i = 0;
            while (i < fools_gold_count) {
                while (1) {
                    unsigned int r = get_random_number();
                    if (gmp->map[r] == 0) {
                        gmp->map[r] = G_FOOL;
                        break;
                    }
                }
                ++i;
            }
        }

        is_good_ = true;
    } else {
        is_good_ = false;
    }
}