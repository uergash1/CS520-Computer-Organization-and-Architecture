/*
 *  lsq_driver.h
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */

int
is_lsq_entry_free(APEX_CPU* cpu);

int
push_lsq_entry(APEX_CPU* cpu, LSQ_Entry* new_lsq_entry);

void
get_instruction_to_MEM(APEX_CPU* cpu);

void
update_lsq_entry(APEX_CPU* cpu, enum STAGES FU_type);

void
broadcast_result_into_lsq(APEX_CPU* cpu, enum STAGES FU_type);

void
flush_lsq(APEX_CPU* cpu, int branch_id);

void
process_lsq(APEX_CPU* cpu);

void
display_lsq(APEX_CPU* cpu);
