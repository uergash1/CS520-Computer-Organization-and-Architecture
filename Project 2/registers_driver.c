/*
 *  registers_driver.c
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

int
is_phys_reg_free(APEX_CPU* cpu)
{
  for (int i = 0; i < URF_ENTRIES_NUMBER; i++) {
    if (cpu->urf[i].free) {
      return 1;
    }
  }
  return 0; // there is NO free physical register
}

int
get_phys_reg(APEX_CPU* cpu)
{
  for (int i = 0; i < URF_ENTRIES_NUMBER; i++) {
    if (cpu->urf[i].free) {
      return i;
    }
  }
  return -1;
}

// Before calling this function, make sure you first call
// is_phys_reg_free function explicitly
int
allocate_phys_reg(APEX_CPU* cpu, int arch_reg)
{
  int free_phys_reg = get_phys_reg(cpu);
  cpu->urf[free_phys_reg].free = 0;   // this phys reg is not free now
  cpu->urf[free_phys_reg].valid = 0;  // this phys reg is not valid now
  cpu->rat[arch_reg].phys_reg = free_phys_reg;
  return free_phys_reg;
}

void
deallocate_phys_reg(APEX_CPU* cpu, int phys_reg)
{
  //int phys_reg = cpu->rrat[arch_reg].commited_phys_reg;
  if (phys_reg != -1) {
    //printf("Releasing phys_reg: %d, arch_reg: %d\n", phys_reg, arch_reg);
    cpu->urf[phys_reg].free = 1;
  }
}

void
commit_register(APEX_CPU* cpu, int arch_reg, int phys_reg)
{
  //printf("In commit_register #1: %d\n", cpu->clock);
  int phys_reg_to_deallocate = cpu->rrat[arch_reg].commited_phys_reg;
  deallocate_phys_reg(cpu, phys_reg_to_deallocate);
  //printf("Commiting phys_reg: %d\n", phys_reg);
  cpu->rrat[arch_reg].commited_phys_reg = phys_reg;
}

void
rename_source1(APEX_CPU* cpu)
{
  //printf("------\n");
  CPU_Stage* stage = &cpu->stage[DRF];
  int arch_rs1 = stage->arch_rs1;
  //printf("Got arch_rs1: %d\n", arch_rs1);
  stage->phys_rs1 = cpu->rat[arch_rs1].phys_reg;
  //printf("Renamed phys_rs1: %d\n", stage->phys_rs1);
}

void
rename_source2(APEX_CPU* cpu)
{
  //printf("------\n");
  CPU_Stage* stage = &cpu->stage[DRF];
  int arch_rs2 = stage->arch_rs2;
  //printf("Got arch_rs2: %d\n", arch_rs2);
  stage->phys_rs2 = cpu->rat[arch_rs2].phys_reg;
  //printf("Renamed phys_rs2: %d\n", stage->phys_rs2);
}

void
read_source1(APEX_CPU* cpu)
{
  //printf("------\n");
  CPU_Stage* stage = &cpu->stage[DRF];
  int phys_rs1 = stage->phys_rs1;
  if (cpu->urf[phys_rs1].valid) {
    stage->rs1_value = cpu->urf[phys_rs1].value;
    stage->rs1_valid = 1;
    //printf("phys_rs1: %d\n", phys_rs1);
    //printf("stage->rs1_value: %d\n", stage->rs1_value);
  }
}

void
read_source2(APEX_CPU* cpu)
{
  //printf("------\n");
  CPU_Stage* stage = &cpu->stage[DRF];
  int phys_rs2 = stage->phys_rs2;
  if (cpu->urf[phys_rs2].valid) {
    stage->rs2_value = cpu->urf[phys_rs2].value;
    stage->rs2_valid = 1;
    //printf("phys_rs2: %d\n", phys_rs2);
    //printf("stage->rs2_value: %d\n", stage->rs2_value);
  }
}

void
write_urf(APEX_CPU* cpu, enum STAGES FU_type)
{
  int phys_reg = cpu->stage[FU_type].phys_rd;
  int result = cpu->stage[FU_type].buffer;
  cpu->urf[phys_reg].value = result;
  cpu->urf[phys_reg].valid = 1;
}

void
save_urf_rat(APEX_CPU* cpu, int branch_id)
{
  //for (int j=0; j < URF_ENTRIES_NUMBER; j++) {
    //cpu->bis.backup_entry[branch_id].urf[j].value = cpu->urf[j].value;
    //cpu->bis.backup_entry[branch_id].urf[j].free = cpu->urf[j].free;
    //cpu->bis.backup_entry[branch_id].urf[j].valid = cpu->urf[j].valid;
  //}
  for (int j=0; j < RAT_ENTRIES_NUMBER; j++) {
    cpu->bis.backup_entry[branch_id].rat[j].phys_reg = cpu->rat[j].phys_reg;
  }
}

void
recover_urf_rat(APEX_CPU* cpu)
{
  int branch_id = cpu->stage[Int_FU].branch_id;
  //for (int j=0; j < URF_ENTRIES_NUMBER; j++) {
    //cpu->urf[j].value = cpu->bis.backup_entry[branch_id].urf[j].value;
    //cpu->urf[j].free = cpu->bis.backup_entry[branch_id].urf[j].free;
    //cpu->urf[j].valid = cpu->bis.backup_entry[branch_id].urf[j].valid;
  //}
  for (int j=0; j < RAT_ENTRIES_NUMBER; j++) {
    cpu->rat[j].phys_reg = cpu->bis.backup_entry[branch_id].rat[j].phys_reg;
  }
}

void
print_urf_for_debug(APEX_CPU* cpu)
{
  printf("------------------------------ Details of URF State -----------------------------\n");
  for (int i = 0; i < URF_ENTRIES_NUMBER; i++) {
    if (!cpu->urf[i].free) {
      printf("| URF[%d] = %d, VALID = %d |",
              i, cpu->urf[i].value, cpu->urf[i].valid);
    }
  }
  printf("\n");
  printf("---------------------------------------------------------------------------------\n\n");
}

void
print_datamemory_for_debug(APEX_CPU* cpu)
{
  printf("-------------------------- Details of Data Memory State -------------------------\n");
  for (int i = 0; i < 100; i++) {
    if (cpu->data_memory[i]) {
      printf("| D[%d] = %d |",
              i, cpu->data_memory[i]);
    }
  }
  printf("\n");
  printf("---------------------------------------------------------------------------------\n\n");
}

void
display_rat(APEX_CPU* cpu)
{
  int rat_empty = 1;
  printf("-------------------------------------- RAT --------------------------------------\n");
  for (int i = 0; i < RAT_ENTRIES_NUMBER; i++) {
    if (cpu->rat[i].phys_reg != -1) {
      rat_empty = 0;
      printf("| RAT[%d] = U%d |",
              i, cpu->rat[i].phys_reg);
    }
  }
  if (rat_empty) {
    printf("Empty");
  }
  printf("\n");
  printf("---------------------------------------------------------------------------------\n\n");
}

void
display_rrat(APEX_CPU* cpu)
{
  int rrat_empty = 1;
  printf("------------------------------------- R-RAT -------------------------------------\n");
  for (int i = 0; i < RRAT_ENTRIES_NUMBER; i++) {
    if (cpu->rrat[i].commited_phys_reg != -1) {
      rrat_empty = 0;
      printf("| R-RAT[%d] = U%d |",
              i, cpu->rrat[i].commited_phys_reg);
    }
  }
  if (rrat_empty) {
    printf("Empty");
  }
  printf("\n");
  printf("---------------------------------------------------------------------------------\n\n");
}

/*void
print_saved_urf(APEX_CPU* cpu, int branch_id)
{
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  printf("Details of URF State for Branch ID: %d\n", branch_id);
  for (int i = 0; i < URF_ENTRIES_NUMBER; i++) {
    if (!cpu->bis.backup_entry[branch_id].urf[i].free) {
      printf("| URF[%d] = %d, VALID = %d |",
              i, cpu->bis.backup_entry[branch_id].urf[i].value, cpu->bis.backup_entry[branch_id].urf[i].valid);
    }
  }
  printf("\n");
}*/

void
print_saved_rat(APEX_CPU* cpu, int branch_id)
{
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  printf("Details of saved RAT State for Branch ID: %d\n", branch_id);
  for (int i = 0; i < RAT_ENTRIES_NUMBER; i++) {
    if (cpu->bis.backup_entry[branch_id].rat[i].phys_reg != -1) {
      printf("| RAT[%d] = U%d |",
              i, cpu->bis.backup_entry[branch_id].rat[i].phys_reg);
    }
  }
  printf("\n");
}

void
display_registers(APEX_CPU* cpu)
{
  display_rrat(cpu);
  display_rat(cpu);
  //print_urf_for_debug(cpu);
  //print_datamemory_for_debug(cpu);
}

void
display_urf(APEX_CPU* cpu)
{
  printf("\n======================== STATE OF UNIFIED REGISTER FILE ========================\n");
  for (int i = 0; i < URF_ENTRIES_NUMBER; i++) {
    if (!cpu->urf[i].free) {
      printf("         |\tURF[%d]\t|\tValue = %d\t|\tStatus = %d\t|\n",
            i, cpu->urf[i].value, cpu->urf[i].valid);
    }
  }
  printf("================================================================================\n");
}

void
display_data_mem(APEX_CPU* cpu)
{
  printf("\n============================= STATE OF DATA MEMORY =============================\n");
  for (int i = 0; i < 100; i++) {
    printf("                     |\tMEM[%d]\t|\tData Value = %d\t|\n",
            i, cpu->data_memory[i]);
  }
  printf("================================================================================\n\n");
}

void
display_regs_mem(APEX_CPU* cpu)
{
  display_urf(cpu);
  display_data_mem(cpu);
}
