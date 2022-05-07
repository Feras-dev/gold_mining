#ifndef __MINE_ENTRANCE_H__
#define __MINE_ENTRANCE_H__

// game shared data
struct goldMine_S {
    unsigned short total_num_gold;
    unsigned short rows;
    unsigned short cols;
    unsigned char  players;
    unsigned char  map[];
};

#endif // __MINE_ENTRANCE_H__