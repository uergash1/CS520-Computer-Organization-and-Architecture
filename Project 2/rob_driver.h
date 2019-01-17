/*
 *  rob_driver.h
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */

int
is_rob_entry_free(APEX_CPU* cpu);

int
push_rob_entry(APEX_CPU* cpu, ROB_Entry* new_rob_entry);

int
commit_rob_entry(APEX_CPU* cpu);

int
update_rob_entry(APEX_CPU* cpu, enum STAGES FU_type);

void
remove_store_from_rob(APEX_CPU* cpu);

void
flush_rob(APEX_CPU* cpu);

void
display_rob(APEX_CPU* cpu);
