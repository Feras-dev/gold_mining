#ifndef __ERROR_HANDLER_H__
#define __ERROR_HANDLER_H__

enum ERROR_CODE_E {
    ok,
    error_map_file_specified_by_subsequent_player,
    error_no_map_file_specified_by_first_player,
    error_map_file_specified_is_not_valid,
    error_map_constructor_threw_an_exception,
    error_in_shm_open,
    error_in_ftruncate,
    error_in_mmap,
    error_illegal_charecter_in_map_file,
    error_max_number_of_players_reached,
    error_in_sem_close,
    error_in_sem_unlink,
    error_in_shm_unlink,
    error_in_sem_wait,
    error_in_sem_post,
    error_failed_initialization,
    error_failed_map_rendering,
    error_,
    count_of_error_codes
};

void handle_error(ERROR_CODE_E error_code);

#endif // __ERROR_HANDLER_H__