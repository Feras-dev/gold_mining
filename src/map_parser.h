#ifndef __MAP_PARSER_H__
#define __MAP_PARSER_H__

#include <string>

#include "mine_entrance.h"

#define REAL_GOLD_COUNT 1

class Map_parser {
  private:
    unsigned int fools_gold_count = 0;
    unsigned int total_gold_count = 0;
    unsigned int rows             = 0;
    unsigned int columns          = 0;
    std::string  map_file_path    = "";
    bool         is_good_         = false;

  public:
    Map_parser(std::string path_to_map_file);
    ~Map_parser();
    bool         is_good();
    unsigned int get_rows();
    unsigned int get_cols();
    unsigned int get_count_of_total_gold();
    unsigned int get_count_of_fools_gold();
    unsigned int get_random_number();
    void         slurp_map(goldMine_S *gmp);
};

#endif // __MAP_PARSER_H__