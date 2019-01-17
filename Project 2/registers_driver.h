/*
 *  registers_driver.h
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */

int
is_phys_reg_free(APEX_CPU* cpu);

int
allocate_phys_reg(APEX_CPU* cpu, int arch_reg);

void
deallocate_phys_reg(APEX_CPU* cpu, int phys_reg);

void
commit_register(APEX_CPU* cpu, int arch_reg, int phys_reg);

void
rename_source1(APEX_CPU* cpu);

void
rename_source2(APEX_CPU* cpu);

void
read_source1(APEX_CPU* cpu);

void
read_source2(APEX_CPU* cpu);

void
save_urf_rat(APEX_CPU* cpu, int bis_id);

void
recover_urf_rat(APEX_CPU* cpu);

void
write_urf(APEX_CPU* cpu, enum STAGES FU_type);

void
display_registers(APEX_CPU* cpu);

void
print_saved_rat(APEX_CPU* cpu, int branch_id);

//void
//print_saved_urf(APEX_CPU* cpu, int branch_id);

void
display_regs_mem(APEX_CPU* cpu);
