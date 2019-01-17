/*
 *  iq_driver.c
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "iq_driver.h"

int
is_iq_entry_free(APEX_CPU* cpu)
{
  for (int i = 0; i < IQ_ENTRIES_NUMBER; i++) {
    if (cpu->iq.iq_entry[i].free) {
      cpu->iq.free_entry = i;
      return 1;
    }
  }
  return 0;
}

// Before calling this function, make sure you first call
// is_iq_entry_free function explicitly
int
push_iq_entry(APEX_CPU* cpu, ISSUE_QUEUE_Entry* new_iq_entry)
{
  int free_entry = cpu->iq.free_entry;
  cpu->iq.iq_entry[free_entry].pc = new_iq_entry->pc;
  strcpy(cpu->iq.iq_entry[free_entry].opcode, new_iq_entry->opcode);
  cpu->iq.iq_entry[free_entry].counter  = new_iq_entry->counter;
  cpu->iq.iq_entry[free_entry].free  = new_iq_entry->free;
  cpu->iq.iq_entry[free_entry].FU_type  = new_iq_entry->FU_type;
  cpu->iq.iq_entry[free_entry].imm  = new_iq_entry->imm;
  cpu->iq.iq_entry[free_entry].rs1_ready  = new_iq_entry->rs1_ready;
  cpu->iq.iq_entry[free_entry].phys_rs1  = new_iq_entry->phys_rs1;
  cpu->iq.iq_entry[free_entry].arch_rs1  = new_iq_entry->arch_rs1;
  cpu->iq.iq_entry[free_entry].rs1_value  = new_iq_entry->rs1_value;
  cpu->iq.iq_entry[free_entry].rs2_ready  = new_iq_entry->rs2_ready;
  cpu->iq.iq_entry[free_entry].phys_rs2  = new_iq_entry->phys_rs2;
  cpu->iq.iq_entry[free_entry].arch_rs2  = new_iq_entry->arch_rs2;
  cpu->iq.iq_entry[free_entry].rs2_value  = new_iq_entry->rs2_value;
  cpu->iq.iq_entry[free_entry].phys_rd  = new_iq_entry->phys_rd;
  cpu->iq.iq_entry[free_entry].arch_rd  = new_iq_entry->arch_rd;
  cpu->iq.iq_entry[free_entry].LSQ_index  = new_iq_entry->LSQ_index;
  cpu->iq.iq_entry[free_entry].rob_entry_id  = new_iq_entry->rob_entry_id;
  cpu->iq.iq_entry[free_entry].branch_id  = new_iq_entry->branch_id;
  return 0;
}

int
get_instruction_for_FUs(APEX_CPU* cpu, enum STAGES FU_Type)
{
  int process = 1;
  if (FU_Type == Mul_FU && cpu->stage[Mul_FU].stalled) {
    process = 0;
  }

  if (process) {
    int issue_instruction_index = -1;
    int max_counter = 0;
    for (int i = 0; i < IQ_ENTRIES_NUMBER; i++) {

      if (!cpu->iq.iq_entry[i].free &&
          cpu->iq.iq_entry[i].FU_type == FU_Type &&
          cpu->iq.iq_entry[i].rs1_ready &&
          cpu->iq.iq_entry[i].rs2_ready &&
          cpu->iq.iq_entry[i].counter > max_counter) {

        if (strcmp(cpu->iq.iq_entry[i].opcode, "BZ") == 0 ||
            strcmp(cpu->iq.iq_entry[i].opcode, "BNZ") == 0) {

          int branch_id = cpu->iq.iq_entry[i].branch_id;
          int branch_phys_src = cpu->bis.bis_entry[branch_id].phys_src;
          if (cpu->urf[branch_phys_src].valid) {
            max_counter = cpu->iq.iq_entry[i].counter;
            issue_instruction_index = i;
          }
        }
        else {
          max_counter = cpu->iq.iq_entry[i].counter;
          issue_instruction_index = i;
        }
      }
    }

    if (max_counter != 0) {
      //CPU_Stage* Int_FU_stage;
      cpu->stage[FU_Type].pc = cpu->iq.iq_entry[issue_instruction_index].pc;
      strcpy(cpu->stage[FU_Type].opcode, cpu->iq.iq_entry[issue_instruction_index].opcode);
      cpu->stage[FU_Type].arch_rs1 = cpu->iq.iq_entry[issue_instruction_index].arch_rs1;
      cpu->stage[FU_Type].arch_rs2 = cpu->iq.iq_entry[issue_instruction_index].arch_rs2;
      cpu->stage[FU_Type].phys_rs1 = cpu->iq.iq_entry[issue_instruction_index].phys_rs1;
      cpu->stage[FU_Type].phys_rs2 = cpu->iq.iq_entry[issue_instruction_index].phys_rs2;
      cpu->stage[FU_Type].phys_rd = cpu->iq.iq_entry[issue_instruction_index].phys_rd;
      cpu->stage[FU_Type].arch_rd = cpu->iq.iq_entry[issue_instruction_index].arch_rd;
      cpu->stage[FU_Type].imm = cpu->iq.iq_entry[issue_instruction_index].imm;
      cpu->stage[FU_Type].rs1_value = cpu->iq.iq_entry[issue_instruction_index].rs1_value;
      cpu->stage[FU_Type].rs2_value = cpu->iq.iq_entry[issue_instruction_index].rs2_value;
      cpu->stage[FU_Type].rob_entry_id = cpu->iq.iq_entry[issue_instruction_index].rob_entry_id;
      cpu->stage[FU_Type].branch_id = cpu->iq.iq_entry[issue_instruction_index].branch_id;
      cpu->stage[FU_Type].LSQ_index = cpu->iq.iq_entry[issue_instruction_index].LSQ_index;
      cpu->stage[FU_Type].busy = 0;
      cpu->stage[FU_Type].stalled = 0;

      // Clearing IQ entry
      cpu->iq.iq_entry[issue_instruction_index].free = 1;
    }
  }

  return 0;
}

int
update_counters(APEX_CPU* cpu)
{
  for (int i = 0; i < IQ_ENTRIES_NUMBER; i++) {
    if (!cpu->iq.iq_entry[i].free) {
      cpu->iq.iq_entry[i].counter++;
    }
  }
  return 0;
}

int
broadcast_result_into_iq(APEX_CPU* cpu, enum STAGES FU_type)
{
  for (int i = 0; i < IQ_ENTRIES_NUMBER; i++) {
    if (!cpu->iq.iq_entry[i].free) {
      if (cpu->iq.iq_entry[i].phys_rs1 == cpu->stage[FU_type].phys_rd) {
        cpu->iq.iq_entry[i].rs1_value = cpu->stage[FU_type].buffer;
        cpu->iq.iq_entry[i].rs1_ready = 1;
      }

      if (cpu->iq.iq_entry[i].phys_rs2 == cpu->stage[FU_type].phys_rd) {
        cpu->iq.iq_entry[i].rs2_value = cpu->stage[FU_type].buffer;
        cpu->iq.iq_entry[i].rs2_ready = 1;
      }
    }
  }
  return 0;
}

void
print_iq_for_debug(APEX_CPU* cpu)
{
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  printf("Details of IQ State\n");
  for (int i = 0; i < IQ_ENTRIES_NUMBER; i++) {
    if (!cpu->iq.iq_entry[i].free) {
      printf("| ID=%d, PC=%d, OPCODE=%s, COUNTER=%d, FREE=%d, FU_Type=%d, IMM=%d, RS1_READY=%d, PHYS_RS1=%d, RS1_VALUE=%d, RS2_READY=%d, PHYS_RS2=%d, RS2_VALUE=%d, PHYS_RD=%d, ROB_ENTRY=%d, LSQ=%d, BRCH_ID=%d |\n",
              i, cpu->iq.iq_entry[i].pc, cpu->iq.iq_entry[i].opcode, cpu->iq.iq_entry[i].counter,
              cpu->iq.iq_entry[i].free, cpu->iq.iq_entry[i].FU_type, cpu->iq.iq_entry[i].imm,
              cpu->iq.iq_entry[i].rs1_ready, cpu->iq.iq_entry[i].phys_rs1, cpu->iq.iq_entry[i].rs1_value,
              cpu->iq.iq_entry[i].rs2_ready, cpu->iq.iq_entry[i].phys_rs2, cpu->iq.iq_entry[i].rs2_value,
              cpu->iq.iq_entry[i].phys_rd, cpu->iq.iq_entry[i].rob_entry_id, cpu->iq.iq_entry[i].LSQ_index,
              cpu->iq.iq_entry[i].branch_id);
    }
  }
}

void
display_iq(APEX_CPU* cpu)
{
  int iq_empty = 1;
  printf("\n--------------------------------- Issue Queue -----------------------------------\n");
  for (int i = 0; i < IQ_ENTRIES_NUMBER; i++) {
    if (!cpu->iq.iq_entry[i].free) {
      iq_empty = 0;
      printf("| Counter = %d |\tpc(%d)  ", cpu->iq.iq_entry[i].counter, cpu->iq.iq_entry[i].pc);
      CPU_Stage* instruction_to_print = malloc(sizeof(*instruction_to_print));
      strcpy(instruction_to_print->opcode, cpu->iq.iq_entry[i].opcode);
      instruction_to_print->arch_rs1 = cpu->iq.iq_entry[i].arch_rs1;
      instruction_to_print->phys_rs1 = cpu->iq.iq_entry[i].phys_rs1;
      instruction_to_print->arch_rs2 = cpu->iq.iq_entry[i].arch_rs2;
      instruction_to_print->phys_rs2 = cpu->iq.iq_entry[i].phys_rs2;
      instruction_to_print->arch_rd = cpu->iq.iq_entry[i].arch_rd;
      instruction_to_print->phys_rd = cpu->iq.iq_entry[i].phys_rd;
      instruction_to_print->imm = cpu->iq.iq_entry[i].imm;
      print_instruction(0, instruction_to_print);
      printf("\t|\n");
    }
  }
  if (iq_empty) {
    printf("Empty\n");
  }
  printf("---------------------------------------------------------------------------------\n\n");
}

void
flush_iq(APEX_CPU* cpu, int branch_id)
{
  if (branch_id <= cpu->last_branch_id) {
    while (branch_id <= cpu->last_branch_id) {
      for (int i = 0; i < IQ_ENTRIES_NUMBER; i++) {
        if (!cpu->iq.iq_entry[i].free && cpu->iq.iq_entry[i].branch_id == branch_id) {
          cpu->iq.iq_entry[i].free = 1;
        }
      }
      branch_id++;
    }
  }
  else {
    while (branch_id < BIS_ENTRIES_NUMBER) {
      for (int i = 0; i < IQ_ENTRIES_NUMBER; i++) {
        if (!cpu->iq.iq_entry[i].free && cpu->iq.iq_entry[i].branch_id == branch_id) {
          cpu->iq.iq_entry[i].free = 1;
        }
      }
      branch_id++;
    }
    branch_id = 0;
    while (branch_id <= cpu->last_branch_id) {
      for (int i = 0; i < IQ_ENTRIES_NUMBER; i++) {
        if (!cpu->iq.iq_entry[i].free && cpu->iq.iq_entry[i].branch_id == branch_id) {
          cpu->iq.iq_entry[i].free = 1;
        }
      }
      branch_id++;
    }
  }
}

int
process_iq(APEX_CPU* cpu)
{
  //print_iq_for_debug(cpu);
  get_instruction_for_FUs(cpu, Int_FU);
  get_instruction_for_FUs(cpu, Mul_FU);
  update_counters(cpu);
  return 0;
}
