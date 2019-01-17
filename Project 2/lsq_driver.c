/*
 *  lsq_driver.c
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "rob_driver.h"


int
is_lsq_entry_free(APEX_CPU* cpu)
{
  if (cpu->lsq.lsq_entry[cpu->lsq.tail].free) {
    return 1;
  }
  return 0;
}

int
push_lsq_entry(APEX_CPU* cpu, LSQ_Entry* new_lsq_entry)
{
  int free_entry = cpu->lsq.tail;
  cpu->lsq.lsq_entry[free_entry].free = new_lsq_entry->free;
  strcpy(cpu->lsq.lsq_entry[free_entry].opcode, new_lsq_entry->opcode);
  cpu->lsq.lsq_entry[free_entry].pc = new_lsq_entry->pc;
  cpu->lsq.lsq_entry[free_entry].mem_address_valid = new_lsq_entry->mem_address_valid;
  cpu->lsq.lsq_entry[free_entry].mem_address = new_lsq_entry->mem_address;
  cpu->lsq.lsq_entry[free_entry].branch_id = new_lsq_entry->branch_id;
  cpu->lsq.lsq_entry[free_entry].rob_entry_id = new_lsq_entry->rob_entry_id;
  cpu->lsq.lsq_entry[free_entry].rs1_ready = new_lsq_entry->rs1_ready;
  cpu->lsq.lsq_entry[free_entry].phys_rs1 = new_lsq_entry->phys_rs1;
  cpu->lsq.lsq_entry[free_entry].arch_rs1 = new_lsq_entry->arch_rs1;
  cpu->lsq.lsq_entry[free_entry].rs1_value = new_lsq_entry->rs1_value;
  cpu->lsq.lsq_entry[free_entry].phys_rs2 = new_lsq_entry->phys_rs2;
  cpu->lsq.lsq_entry[free_entry].arch_rs2 = new_lsq_entry->arch_rs2;
  cpu->lsq.lsq_entry[free_entry].imm = new_lsq_entry->imm;
  cpu->lsq.lsq_entry[free_entry].arch_rd = new_lsq_entry->arch_rd;
  cpu->lsq.lsq_entry[free_entry].phys_rd = new_lsq_entry->phys_rd;

  cpu->lsq.tail++;
  if (cpu->lsq.tail == LSQ_ENTRIES_NUMBER) {
    cpu->lsq.tail = 0;
  }
  return free_entry;
}

void
get_instruction_to_MEM(APEX_CPU* cpu)
{
  int push_to_mem = 0;
  int entry = cpu->lsq.head;
  if (!cpu->lsq.lsq_entry[entry].free &&
      cpu->lsq.lsq_entry[entry].mem_address_valid &&
      !cpu->stage[MEM].stalled) {

    if (strcmp(cpu->lsq.lsq_entry[entry].opcode, "STORE") == 0) {
      if (cpu->lsq.lsq_entry[entry].rs1_ready && cpu->commitments != 2) {
        int rob_head = cpu->rob.head;
        if (strcmp(cpu->rob.rob_entry[rob_head].opcode, "STORE") == 0) {
          push_to_mem = 1;
          // delete from ROB
          remove_store_from_rob(cpu);
        }
      }
    }
    else {
      push_to_mem = 1;
    }
  }

  if (push_to_mem) {
    cpu->stage[MEM].pc = cpu->lsq.lsq_entry[entry].pc;
    strcpy(cpu->stage[MEM].opcode, cpu->lsq.lsq_entry[entry].opcode);
    cpu->stage[MEM].arch_rd = cpu->lsq.lsq_entry[entry].arch_rd;
    cpu->stage[MEM].phys_rd = cpu->lsq.lsq_entry[entry].phys_rd;
    cpu->stage[MEM].phys_rs1 = cpu->lsq.lsq_entry[entry].phys_rs1;
    cpu->stage[MEM].arch_rs1 = cpu->lsq.lsq_entry[entry].arch_rs1;
    cpu->stage[MEM].phys_rs2 = cpu->lsq.lsq_entry[entry].phys_rs2;
    cpu->stage[MEM].arch_rs2 = cpu->lsq.lsq_entry[entry].arch_rs2;
    cpu->stage[MEM].imm = cpu->lsq.lsq_entry[entry].imm;
    cpu->stage[MEM].rs1_value = cpu->lsq.lsq_entry[entry].rs1_value;
    cpu->stage[MEM].mem_address = cpu->lsq.lsq_entry[entry].mem_address;
    cpu->stage[MEM].rob_entry_id = cpu->lsq.lsq_entry[entry].rob_entry_id;
    cpu->stage[MEM].branch_id = cpu->lsq.lsq_entry[entry].branch_id;
    cpu->stage[MEM].busy = 0;
    cpu->stage[MEM].stalled = 0;

    cpu->lsq.lsq_entry[entry].free = 1;
    cpu->lsq.head++;
    if (cpu->lsq.head == LSQ_ENTRIES_NUMBER) {
      cpu->lsq.head = 0;
    }
  }
}

void
update_lsq_entry(APEX_CPU* cpu, enum STAGES FU_type)
{
  int LSQ_index = cpu->stage[FU_type].LSQ_index;
  cpu->lsq.lsq_entry[LSQ_index].mem_address = cpu->stage[FU_type].buffer;
  cpu->lsq.lsq_entry[LSQ_index].mem_address_valid = 1;
}

void
broadcast_result_into_lsq(APEX_CPU* cpu, enum STAGES FU_type)
{
  for (int i = 0; i < LSQ_ENTRIES_NUMBER; i++) {
    if (!cpu->lsq.lsq_entry[i].free &&
        cpu->lsq.lsq_entry[i].phys_rs1 == cpu->stage[FU_type].phys_rd) {
      cpu->lsq.lsq_entry[i].rs1_value = cpu->stage[FU_type].buffer;
      cpu->lsq.lsq_entry[i].rs1_ready = 1;
    }
  }
}

void
print_lsq_for_debug(APEX_CPU* cpu)
{
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  printf("Details of LSQ State\n");
  for (int i = 0; i < LSQ_ENTRIES_NUMBER; i++) {
    if (!cpu->lsq.lsq_entry[i].free) {
      printf("| ID=%d, OPCODE=%s, PC=%d, MAV=%d, MA=%d, BR=%d, ROB=%d, RS1_READY=%d, PHYS_RS1=%d, RS1_VALUE=%d, PHYS_RS2=%d, IMM=%d, ARCH_RD=%d PHYS_RD=%d |\n",
              i, cpu->lsq.lsq_entry[i].opcode, cpu->lsq.lsq_entry[i].pc,
              cpu->lsq.lsq_entry[i].mem_address_valid, cpu->lsq.lsq_entry[i].mem_address,
              cpu->lsq.lsq_entry[i].branch_id, cpu->lsq.lsq_entry[i].rob_entry_id,
              cpu->lsq.lsq_entry[i].rs1_ready, cpu->lsq.lsq_entry[i].phys_rs1, cpu->lsq.lsq_entry[i].rs1_value,
              cpu->lsq.lsq_entry[i].phys_rs2, cpu->lsq.lsq_entry[i].imm, cpu->lsq.lsq_entry[i].arch_rd,
              cpu->lsq.lsq_entry[i].phys_rd);
    }
  }
  printf("Tail: %d, Head: %d\n\n", cpu->lsq.tail, cpu->lsq.head);
}

void
display_lsq(APEX_CPU* cpu)
{
  printf("------------------------------- Load Store Queue --------------------------------\n");
  for (int i = 0; i < LSQ_ENTRIES_NUMBER; i++) {
    if (!cpu->lsq.lsq_entry[i].free || i == cpu->lsq.tail) {

      printf("| Index = %d | ", i);

      if (i == cpu->lsq.tail) {
        printf("t |");
      }
      else {
        printf("  |");
      }

      if (i == cpu->lsq.head) {
        printf(" h |\t");
      }
      else {
        printf("   |\t");
      }

      //printf("\t");
      if (!cpu->lsq.lsq_entry[i].free) {
      printf("pc(%d)  ", cpu->lsq.lsq_entry[i].pc);
        CPU_Stage* instruction_to_print = malloc(sizeof(*instruction_to_print));
        strcpy(instruction_to_print->opcode, cpu->lsq.lsq_entry[i].opcode);
        instruction_to_print->arch_rs1 = cpu->lsq.lsq_entry[i].arch_rs1;
        instruction_to_print->phys_rs1 = cpu->lsq.lsq_entry[i].phys_rs1;
        instruction_to_print->arch_rs2 = cpu->lsq.lsq_entry[i].arch_rs2;
        instruction_to_print->phys_rs2 = cpu->lsq.lsq_entry[i].phys_rs2;
        instruction_to_print->arch_rd = cpu->lsq.lsq_entry[i].arch_rd;
        instruction_to_print->phys_rd = cpu->lsq.lsq_entry[i].phys_rd;
        instruction_to_print->imm = cpu->lsq.lsq_entry[i].imm;
        print_instruction(0, instruction_to_print);
        printf("\t|");
      }
      printf("\n");
    }
  }
  printf("---------------------------------------------------------------------------------\n\n");
}

int
is_lsq_empty(APEX_CPU* cpu)
{
  for (int i = 0; i < LSQ_ENTRIES_NUMBER; i++) {
    if (!cpu->lsq.lsq_entry[i].free){
      return 0;
    }
  }
  return 1;
}

void
flush_lsq(APEX_CPU* cpu, int branch_id)
{
  if (branch_id <= cpu->last_branch_id) {
    while (branch_id <= cpu->last_branch_id) {
      for (int i = 0; i < LSQ_ENTRIES_NUMBER; i++) {
        if (!cpu->lsq.lsq_entry[i].free && cpu->lsq.lsq_entry[i].branch_id == branch_id) {
          cpu->lsq.lsq_entry[i].free = 1;
        }
      }
      branch_id++;
    }
  }
  else {
    while (branch_id < BIS_ENTRIES_NUMBER) {
      for (int i = 0; i < LSQ_ENTRIES_NUMBER; i++) {
        if (!cpu->lsq.lsq_entry[i].free && cpu->lsq.lsq_entry[i].branch_id == branch_id) {
          cpu->lsq.lsq_entry[i].free = 1;
        }
      }
      branch_id++;
    }
    branch_id = 0;
    while (branch_id <= cpu->last_branch_id) {
      for (int i = 0; i < LSQ_ENTRIES_NUMBER; i++) {
        if (!cpu->lsq.lsq_entry[i].free && cpu->lsq.lsq_entry[i].branch_id == branch_id) {
          cpu->lsq.lsq_entry[i].free = 1;
        }
      }
      branch_id++;
    }
  }

  if (is_lsq_empty(cpu)) {
    cpu->lsq.tail = 0;
    cpu->lsq.head = 0;
  }
  else {
    //int previous_branch_id = initial_branch_id--;
    int new_tail = cpu->lsq.head;
    while (!cpu->lsq.lsq_entry[new_tail].free) {
      new_tail++;
      if (new_tail == LSQ_ENTRIES_NUMBER) {
        new_tail = 0;
      }
    }
    cpu->lsq.tail = new_tail;
  }
}

void
process_lsq(APEX_CPU* cpu)
{
  get_instruction_to_MEM(cpu);
}
