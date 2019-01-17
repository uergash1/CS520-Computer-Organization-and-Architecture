/*
 *  iq_driver.h
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */

int
is_iq_entry_free(APEX_CPU* cpu);

int
push_iq_entry(APEX_CPU* cpu, ISSUE_QUEUE_Entry* new_iq_entry);

int
get_instruction_for_FUs(APEX_CPU* cpu, enum STAGES FU_Type);

int
update_counters(APEX_CPU* cpu);

int
broadcast_result_into_iq(APEX_CPU* cpu, enum STAGES FU_type);

void
flush_iq(APEX_CPU* cpu, int branch_id);

int
process_iq(APEX_CPU* cpu);

void
display_iq(APEX_CPU* cpu);
