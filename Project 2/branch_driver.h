/*
 *  branch_driver.h
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */

int
is_bis_entry_free(APEX_CPU* cpu);

int
get_bis_entry(APEX_CPU* cpu);

void
deallocate_branch_id(APEX_CPU* cpu, int branch_id);

void
flush_instructions(APEX_CPU* cpu);
