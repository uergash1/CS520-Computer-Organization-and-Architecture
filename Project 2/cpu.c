/*
 *  cpu.c
 *  Contains APEX cpu pipeline implementation
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "registers_driver.h"
#include "iq_driver.h"
#include "rob_driver.h"
#include "branch_driver.h"
#include "lsq_driver.h"

/* Flag to enable debug messages */
int ENABLE_DEBUG_MESSAGES;

/*
 * This function creates and initializes APEX cpu.
 */
APEX_CPU*
APEX_cpu_init(const char* filename, const char* function, const int cycles)
{
  if (!filename && !function && !cycles) {
    return NULL;
  }

  APEX_CPU* cpu = malloc(sizeof(*cpu));
  if (!cpu) {
    return NULL;
  }

  if (strcmp(function, "simulate") == 0) {
    ENABLE_DEBUG_MESSAGES = 0;
  }
  else {
    ENABLE_DEBUG_MESSAGES = 1;
  }

  /* Initialize PC, Registers and all pipeline stages */
  cpu->pc = 4000;

  // Initialize URF
  for (int i=0; i<URF_ENTRIES_NUMBER; i++) {
    cpu->urf[i].value = 0; // initial value for registers
    cpu->urf[i].free = 1; // all phyisical registers are FREE
    cpu->urf[i].valid = 0; // all values are NOT valid
  }

  // Initialize RAT
  for (int i=0; i<RAT_ENTRIES_NUMBER; i++) {
    cpu->rat[i].phys_reg = -1; // initially architectural registers point to nowhere
  }

  // Initialize RRAT
  for (int i=0; i<RRAT_ENTRIES_NUMBER; i++) {
    cpu->rrat[i].commited_phys_reg = -1; // initially commited architectural registers point to nowhere
  }

  // Initialize all Stages
  /*for (enum STAGES stage = F; stage < NUM_STAGES; stage++) {
    cpu->stage[stage].pc = -1;
    strcpy(cpu->stage[stage].opcode, "");
    cpu->stage[stage].arch_rs1 = -1;
    cpu->stage[stage].arch_rs2 = -1;
    cpu->stage[stage].arch_rd = -1;
    cpu->stage[stage].phys_rs1 = -1;
    cpu->stage[stage].phys_rs2 = -1;
    cpu->stage[stage].phys_rd = -1;
    cpu->stage[stage].imm = -1;
    cpu->stage[stage].rs1_value = -1;
    cpu->stage[stage].rs2_value = -1;
    cpu->stage[stage].rs1_valid = 0;
    cpu->stage[stage].rs2_valid = 0;
    cpu->stage[stage].buffer = 0;
    cpu->stage[stage].mem_address = 0;
    cpu->stage[stage].stalled = 0;
    if (stage == F) {
      cpu->stage[stage].busy = 0;
    }
    else {
      cpu->stage[stage].busy = 1;
    }
  }*/

  cpu->lsq.head = 0;
  cpu->lsq.tail = 0;
  for (int i = 0; i < LSQ_ENTRIES_NUMBER; i++) {
    cpu->lsq.lsq_entry[i].free = 1;
    strcpy(cpu->lsq.lsq_entry[i].opcode, "");
    cpu->lsq.lsq_entry[i].pc = -1;
    cpu->lsq.lsq_entry[i].mem_address_valid = 0;
    cpu->lsq.lsq_entry[i].mem_address = 0;
    cpu->lsq.lsq_entry[i].branch_id = -1;
    cpu->lsq.lsq_entry[i].rob_entry_id = -1;
    cpu->lsq.lsq_entry[i].rs1_ready = 0;
    cpu->lsq.lsq_entry[i].phys_rs1 = -1;
    cpu->lsq.lsq_entry[i].rs1_value = 0;
    cpu->lsq.lsq_entry[i].phys_rs2 = -1;
    cpu->lsq.lsq_entry[i].imm = 0;
    cpu->lsq.lsq_entry[i].arch_rd = -1;
    cpu->lsq.lsq_entry[i].phys_rd = -1;
  }

  // Initialize BIS and BIS Entries, Backup Content
  cpu->bis.tail = 0;
  cpu->bis.head = 0;
  for (int i = 0; i < BIS_ENTRIES_NUMBER; i++) {
    cpu->bis.bis_entry[i].free = 1;
    cpu->bis.bis_entry[i].phys_src = -1;
    /*for (int j=0; j < URF_ENTRIES_NUMBER; j++) {
      cpu->bis.backup_entry[i].urf[j].value = 0;
      cpu->bis.backup_entry[i].urf[j].free = 1;
      cpu->bis.backup_entry[i].urf[j].valid = 0;
    }*/
    for (int j=0; j < RAT_ENTRIES_NUMBER; j++) {
      cpu->bis.backup_entry[i].rat[j].phys_reg = -1;
    }
  }

  // Initialize ROB and ROB Entries
  cpu->rob.tail = 0;
  cpu->rob.head = 0;
  for (int i=0; i<ROB_ENTRIES_NUMBER; i++) {
    cpu->rob.rob_entry[i].free = 1;
    cpu->rob.rob_entry[i].pc = -1;
    cpu->rob.rob_entry[i].arch_rd = -1;
    cpu->rob.rob_entry[i].phys_rd = -1;
    cpu->rob.rob_entry[i].status = 0;
  }

  // Initialize IQ and IQ Entries
  cpu->iq.free_entry = -1;
  for (int i=0; i<IQ_ENTRIES_NUMBER; i++) {
    cpu->iq.iq_entry[i].pc = -1;
    strcpy(cpu->iq.iq_entry[i].opcode, "");
    cpu->iq.iq_entry[i].counter = 0;
    cpu->iq.iq_entry[i].free = 1;
    cpu->iq.iq_entry[i].FU_type = -1;
    cpu->iq.iq_entry[i].imm = -1;
    cpu->iq.iq_entry[i].rs1_ready = 0;
    cpu->iq.iq_entry[i].phys_rs1 = -1;
    cpu->iq.iq_entry[i].rs1_value = 0;
    cpu->iq.iq_entry[i].rs2_ready = 0;
    cpu->iq.iq_entry[i].phys_rs2 = -1;
    cpu->iq.iq_entry[i].rs2_value = 0;
    cpu->iq.iq_entry[i].phys_rd = -1;
    cpu->iq.iq_entry[i].LSQ_index = -1;
  }

  //memset(cpu->rat, 0, sizeof(int) * 5);
  //memset(cpu->rrat, 0, sizeof(int) * 5);
  //memset(cpu->stage, 0, sizeof(CPU_Stage) * NUM_STAGES);
  memset(cpu->data_memory, 0, sizeof(int) * 4000);

  /* Parse input file and create code memory */
  cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);

  cpu->clock = 1;
  cpu->fill_in_rob = 1;

  cpu->simulation_completed = 0;
  cpu->commitments = 0;

  if (!cpu->code_memory) {
    free(cpu);
    return NULL;
  }

  if (ENABLE_DEBUG_MESSAGES) {
    fprintf(stderr,
            "APEX_CPU : Initialized APEX CPU, loaded %d instructions\n",
            cpu->code_memory_size);
    fprintf(stderr, "APEX_CPU : Printing Code Memory\n");
    printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode", "rd", "rs1", "rs2", "imm");

    for (int i = 0; i < cpu->code_memory_size; ++i) {
      printf("%-9s %-9d %-9d %-9d %-9d\n",
             cpu->code_memory[i].opcode,
             cpu->code_memory[i].rd,
             cpu->code_memory[i].rs1,
             cpu->code_memory[i].rs2,
             cpu->code_memory[i].imm);
    }
  }

  /* Make all stages busy except Fetch stage, initally to start the pipeline */
  for (int i = 1; i < NUM_STAGES; ++i) {
    cpu->stage[i].busy = 1;
  }

  cpu->code_memory_size = cycles;
  cpu->mul_cycle = 1;
  cpu->mem_cycle = 1;
  cpu->last_branch_id = -1;
  cpu->last_arith_phys_rd = -1;

  return cpu;
}

/*
 * This function de-allocates APEX cpu.
 */
void
APEX_cpu_stop(APEX_CPU* cpu)
{
  free(cpu->code_memory);
  free(cpu);
}

/* Converts the PC(4000 series) into
 * array index for code memory
 */
int
get_code_index(int pc)
{
  return (pc - 4000) / 4;
}

void
print_instruction(int fetch_decode, CPU_Stage* stage)
{
  if (strcmp(stage->opcode, "STORE") == 0) {
    if (fetch_decode) {
      printf("%s,R%d,R%d,#%d ", stage->opcode, stage->arch_rs1, stage->arch_rs2, stage->imm);
    }
    else {
      printf("%s,R%d,R%d,#%d  [%s,U%d,U%d,#%d]",
              stage->opcode, stage->arch_rs1, stage->arch_rs2, stage->imm,
              stage->opcode, stage->phys_rs1, stage->phys_rs2, stage->imm);
    }
  }

  if (strcmp(stage->opcode, "LOAD") == 0) {
    if (fetch_decode) {
      printf("%s,R%d,R%d,#%d ", stage->opcode, stage->arch_rd, stage->arch_rs1, stage->imm);
    }
    else {
      printf("%s,R%d,R%d,#%d  [%s,U%d,U%d,#%d]",
              stage->opcode, stage->arch_rd, stage->arch_rs1, stage->imm,
              stage->opcode, stage->phys_rd, stage->phys_rs1, stage->imm);
    }
  }

  if (strcmp(stage->opcode, "MOVC") == 0) {
    if (fetch_decode) {
      printf("%s,R%d,#%d ", stage->opcode, stage->arch_rd, stage->imm);
    }
    else {
      printf("%s,R%d,#%d  [%s,U%d,#%d]",
              stage->opcode, stage->arch_rd, stage->imm,
              stage->opcode, stage->phys_rd, stage->imm);
    }
  }

  if (strcmp(stage->opcode, "ADD") == 0 ||
      strcmp(stage->opcode, "SUB") == 0 ||
      strcmp(stage->opcode, "AND") == 0 ||
      strcmp(stage->opcode, "OR") == 0 ||
      strcmp(stage->opcode, "EX-OR") == 0 ||
      strcmp(stage->opcode, "MUL") == 0) {

    if (fetch_decode) {
      printf("%s,R%d,R%d,R%d ", stage->opcode, stage->arch_rd, stage->arch_rs1, stage->arch_rs2);
    }
    else {
      printf("%s,R%d,R%d,R%d  [%s,U%d,U%d,U%d]",
              stage->opcode, stage->arch_rd, stage->arch_rs1, stage->arch_rs2,
              stage->opcode, stage->phys_rd, stage->phys_rs1, stage->phys_rs2);
    }
  }

  if (strcmp(stage->opcode, "ADDL") == 0 ||
      strcmp(stage->opcode, "SUBL") == 0) {

    if (fetch_decode) {
      printf("%s,R%d,R%d,#%d ", stage->opcode, stage->arch_rd, stage->arch_rs1, stage->imm);
    }
    else {
      printf("%s,R%d,R%d,#%d  [%s,U%d,U%d,#%d]",
              stage->opcode, stage->arch_rd, stage->arch_rs1, stage->imm,
              stage->opcode, stage->phys_rd, stage->phys_rs1, stage->imm);
    }
  }

  if (strcmp(stage->opcode, "BZ") == 0 ||
      strcmp(stage->opcode, "BNZ") == 0) {
    printf("%s,#%d ", stage->opcode, stage->imm);
  }

  if (strcmp(stage->opcode, "JUMP") == 0) {
    if (fetch_decode) {
      printf("%s,R%d,#%d ", stage->opcode, stage->arch_rs1, stage->imm);
    }
    else {
      printf("%s,R%d,#%d  [%s,U%d,#%d]",
              stage->opcode, stage->arch_rs1, stage->imm,
              stage->opcode, stage->phys_rs1, stage->imm);
    }
  }

  if (strcmp(stage->opcode, "JAL") == 0) {
    if (fetch_decode) {
      printf("%s,R%d,R%d,#%d ", stage->opcode, stage->arch_rd, stage->arch_rs1, stage->imm);
    }
    else {
      printf("%s,R%d,R%d,#%d  [%s,U%d,U%d,#%d]",
              stage->opcode, stage->arch_rd, stage->arch_rs1, stage->imm,
              stage->opcode, stage->phys_rd, stage->phys_rs1, stage->imm);
    }
  }

  if (strcmp(stage->opcode, "HALT") == 0) {
    printf("%s", stage->opcode);
  }
}

/* Debug function which dumps the cpu stage
 * content
 */
static void
print_stage_content(char* name, APEX_CPU* cpu, enum STAGES FU_type)
{
  CPU_Stage* stage = &cpu->stage[FU_type];
  if (strcmp(stage->opcode, "") != 0 && strlen(stage->opcode) <= 5) {
    if (strcmp(name, "Execute_Mul") == 0) {
      printf("%-15s: ", name);
      printf("(cycle-%d) pc(%d) ", cpu->mul_cycle, stage->pc);
      print_instruction(0, stage);
    }
    if (strcmp(name, "Memory") == 0) {
      printf("%-15s: ", name);
      printf("(cycle-%d) pc(%d) ", cpu->mem_cycle, stage->pc);
      print_instruction(0, stage);
    }
    if (strcmp(name, "Execute_Int") == 0) {
      printf("%-15s: pc(%d) ", name, stage->pc);
      print_instruction(0, stage);
    }
    if (strcmp(name, "Fetch") == 0 || strcmp(name, "Decode/RF") == 0) {
      printf("%-15s: pc(%d) ", name, stage->pc);
      print_instruction(1, stage);
    }
  }
  else {
    printf("%-15s:", name);
  }
  printf("\n");
}

/* Exception handler messages
 * Key 0 - Computed effective memory is NOT in the range 0 - 4096
 * Key 1 - Invalid register input
 */
int
exception_handler(int code, char* opcode)
{
  switch(code) {
    case 0: printf("ERROR >> Computed effective memory address for %s is out of 4096 memory range size\n", opcode);
            exit(1);
            break;

    case 1: printf("ERROR >> Invalid register input for %s (Register range is within 0-15)\n", opcode);
            exit(1);
            break;
  }
  return 0;
}

int
broadcast_result(APEX_CPU* cpu, enum STAGES FU_type)
{
  write_urf(cpu, FU_type);
  broadcast_result_into_iq(cpu, FU_type);
  broadcast_result_into_lsq(cpu, FU_type);
  return 0;
}

void
clear_stage(APEX_CPU* cpu, enum STAGES FU_type)
{
  strcpy(cpu->stage[FU_type].opcode, "");
}

void
control_flow(APEX_CPU* cpu)
{
  flush_instructions(cpu);
  recover_urf_rat(cpu);
  cpu->stage[F].busy = 0;
  cpu->stage[DRF].stalled = 0;
  cpu->pc = cpu->stage[Int_FU].target_address;
  cpu->fill_in_rob = 0;
}

int
allowed_dispatch(APEX_CPU* cpu, int dest, int lsq, int branch, int iq)
{
  int free_rob_entry = is_rob_entry_free(cpu);

  // ADD, SUB, MUL, AND, OR, EX-OR, MOVC, ADDL, SUBL
  if (iq && dest && !lsq && !branch) {
    if (free_rob_entry && is_iq_entry_free(cpu) && is_phys_reg_free(cpu)) {
      return 1;
    }
  }

  // BZ, BNZ, JUMP
  if (iq && !dest && !lsq && branch) {
    if (free_rob_entry && is_iq_entry_free(cpu) && is_bis_entry_free(cpu)) {
      return 1;
    }
  }

  // JAL
  if (iq && dest && !lsq && branch) {
    if (free_rob_entry && is_iq_entry_free(cpu) && is_phys_reg_free(cpu) && is_bis_entry_free(cpu)) {
      return 1;
    }
  }

  // LOAD
  if (iq && dest && lsq && !branch) {
    if (free_rob_entry && is_iq_entry_free(cpu) && is_phys_reg_free(cpu) && is_lsq_entry_free(cpu)) {
      return 1;
    }
  }

  // STORE
  if (iq && !dest && lsq && !branch) {
    if (free_rob_entry && is_iq_entry_free(cpu) && is_lsq_entry_free(cpu)) {
      return 1;
    }
  }

  // HALT
  if (free_rob_entry && !iq && !dest && !lsq && !branch) {
    return 1;
  }

  return 0;
}

int
dispatch_instruction(APEX_CPU* cpu, int dest, int src1, int src2, int lsq, int branch, enum STAGES FU_type)
{
  CPU_Stage* stage = &cpu->stage[DRF];
  //printf("*** %d %s to IQ\n", stage->pc, stage->opcode);

  if (src1) {
    rename_source1(cpu);
    read_source1(cpu);
  }

  if (src2) {
    rename_source2(cpu);
    read_source2(cpu);
  }

  if (dest) {
    // Allocating Physical Register
    stage->phys_rd = allocate_phys_reg(cpu, stage->arch_rd);
  }

  if (strcmp(stage->opcode, "ADD") == 0 ||
      strcmp(stage->opcode, "SUB") == 0 ||
      strcmp(stage->opcode, "MUL") == 0 ||
      strcmp(stage->opcode, "ADDL") == 0 ||
      strcmp(stage->opcode, "SUBL") == 0) {

    cpu->last_arith_phys_rd = stage->phys_rd;
  }

  if (branch) {
    cpu->last_branch_id = get_bis_entry(cpu);
    save_urf_rat(cpu, cpu->last_branch_id);
  }

  // Pushing ROB Entry
  ROB_Entry* new_rob_entry = malloc(sizeof(*new_rob_entry));
  new_rob_entry->free = 0;
  strcpy(new_rob_entry->opcode, stage->opcode);
  new_rob_entry->pc = stage->pc;
  new_rob_entry->arch_rd = stage->arch_rd;
  new_rob_entry->phys_rd = stage->phys_rd;
  new_rob_entry->arch_rs1 = stage->arch_rs1;
  new_rob_entry->phys_rs1 = stage->phys_rs1;
  new_rob_entry->arch_rs2 = stage->arch_rs2;
  new_rob_entry->phys_rs2 = stage->phys_rs2;
  new_rob_entry->imm = stage->imm;
  if (strcmp(stage->opcode, "HALT") == 0) { new_rob_entry->status = 1; }
  else { new_rob_entry->status = 0; }
  new_rob_entry->branch_id = cpu->last_branch_id;
  stage->rob_entry_id = push_rob_entry(cpu, new_rob_entry);

  if (lsq) {
    LSQ_Entry* new_lsq_entry = malloc(sizeof(*new_lsq_entry));
    new_lsq_entry->free = 0;
    strcpy(new_lsq_entry->opcode, stage->opcode);
    new_lsq_entry->pc = stage->pc;
    new_lsq_entry->mem_address_valid = 0;
    new_lsq_entry->mem_address = 0;
    new_lsq_entry->branch_id = cpu->last_branch_id;
    new_lsq_entry->rob_entry_id = stage->rob_entry_id;
    new_lsq_entry->rs1_ready = stage->rs1_valid;
    new_lsq_entry->phys_rs1 = stage->phys_rs1;
    new_lsq_entry->arch_rs1 = stage->arch_rs1;
    new_lsq_entry->rs1_value = stage->rs1_value;
    new_lsq_entry->phys_rs2 = stage->phys_rs2;
    new_lsq_entry->arch_rs2 = stage->arch_rs2;
    new_lsq_entry->imm = stage->imm;
    new_lsq_entry->arch_rd = stage->arch_rd;
    new_lsq_entry->phys_rd = stage->phys_rd;
    stage->LSQ_index = push_lsq_entry(cpu, new_lsq_entry);
  }

  // Pushing IQ Entry
  if (strcmp(stage->opcode, "HALT") != 0) {
    ISSUE_QUEUE_Entry* new_iq_entry = malloc(sizeof(*new_iq_entry));
    new_iq_entry->pc = stage->pc;
    strcpy(new_iq_entry->opcode, stage->opcode);
    new_iq_entry->counter = 1;
    new_iq_entry->free = 0;
    new_iq_entry->FU_type = FU_type;
    new_iq_entry->imm = stage->imm;
    new_iq_entry->arch_rs1 = stage->arch_rs1;
    if (!src1) { new_iq_entry->rs1_ready = 1; }
    else { new_iq_entry->rs1_ready = stage->rs1_valid; }
    new_iq_entry->phys_rs1 = stage->phys_rs1;
    new_iq_entry->rs1_value = stage->rs1_value;
    new_iq_entry->arch_rs2 = stage->arch_rs2;
    if (!src2) { new_iq_entry->rs2_ready = 1; }
    else { new_iq_entry->rs2_ready = stage->rs2_valid; }
    new_iq_entry->phys_rs2 = stage->phys_rs2;
    new_iq_entry->rs2_value = stage->rs2_value;
    new_iq_entry->arch_rd = stage->arch_rd;
    new_iq_entry->phys_rd = stage->phys_rd;
    new_iq_entry->LSQ_index = stage->LSQ_index;
    new_iq_entry->branch_id = cpu->last_branch_id;
    new_iq_entry->rob_entry_id = stage->rob_entry_id;
    push_iq_entry(cpu, new_iq_entry);
  }

  return 0;
}

int
fetch(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[F];
  if (!stage->busy && !stage->stalled) {

    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
    stage->pc = cpu->pc;
    strcpy(stage->opcode, current_ins->opcode);
    stage->arch_rs1 = current_ins->rs1;
    stage->arch_rs2 = current_ins->rs2;
    stage->arch_rd = current_ins->rd;
    stage->phys_rs1 = -1;
    stage->phys_rs2 = -1;
    stage->phys_rd = -1;
    stage->imm = current_ins->imm;
    stage->rs1_value = 0;
    stage->rs2_value = 0;
    stage->rs1_valid = 0;
    stage->rs2_valid = 0;
    stage->buffer = 0;
    stage->mem_address = 0;
    stage->rob_entry_id = -1;
    stage->branch_id = -1;
    stage->LSQ_index = -1;
    stage->target_address = 0;

    /* Update PC for next instruction */
    cpu->pc += 4;

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Fetch", cpu, F);
    }

    /* Copy data from fetch latch to decode latch */
    if (!cpu->stage[DRF].stalled) {
      cpu->stage[DRF] = cpu->stage[F];
    }
    else {
      stage->stalled = 1;
    }

    //printf("*** Fetch: stalled=%d, busy=%d\n", stage->stalled, stage->busy);
  }
  else {

    if (stage->stalled && strcmp(stage->opcode, "") == 0) {
      stage->stalled = 0;
    }

    if (!cpu->stage[DRF].stalled) {
      stage->stalled = 0;
      cpu->stage[DRF] = cpu->stage[F];
    }

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Fetch", cpu, F);
    }
    //printf("*** Fetch: stalled=%d, busy=%d\n", stage->stalled, stage->busy);
  }
  return 0;
}


int
decode(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[DRF];
  if (!stage->busy && !stage->stalled) {

    if (strcmp(stage->opcode, "MOVC") == 0) {
      if (allowed_dispatch(cpu, 1, 0, 0, 1)) {
        dispatch_instruction(cpu, 1, 0, 0, 0, 0, Int_FU);
      }
      else {
        stage->stalled = 1;
      }
    }

    if (strcmp(stage->opcode, "MUL") == 0) {
      if (allowed_dispatch(cpu, 1, 0, 0, 1)) {
        dispatch_instruction(cpu, 1, 1, 1, 0, 0, Mul_FU);
      }
      else {
        stage->stalled = 1;
      }
    }

    if (strcmp(stage->opcode, "ADD") == 0 ||
        strcmp(stage->opcode, "SUB") == 0 ||
        strcmp(stage->opcode, "AND") == 0 ||
        strcmp(stage->opcode, "OR") == 0 ||
        strcmp(stage->opcode, "EX-OR") == 0) {

      if (allowed_dispatch(cpu, 1, 0, 0, 1)) {
        dispatch_instruction(cpu, 1, 1, 1, 0, 0, Int_FU);
      }
      else {
        stage->stalled = 1;
      }
    }

    if (strcmp(stage->opcode, "ADDL") == 0 ||
        strcmp(stage->opcode, "SUBL") == 0) {
      if (allowed_dispatch(cpu, 1, 0, 0, 1)) {
        dispatch_instruction(cpu, 1, 1, 0, 0, 0, Int_FU);
      }
      else {
        stage->stalled = 1;
      }
    }

    if (strcmp(stage->opcode, "BZ") == 0 ||
        strcmp(stage->opcode, "BNZ") == 0) {
      if (allowed_dispatch(cpu, 0, 0, 1, 1)) {
        dispatch_instruction(cpu, 0, 0, 0, 0, 1, Int_FU);
      }
      else {
        stage->stalled = 1;
      }
    }

    if (strcmp(stage->opcode, "STORE") == 0) {
      if (allowed_dispatch(cpu, 0, 1, 0, 1)) {
        dispatch_instruction(cpu, 0, 1, 1, 1, 0, Int_FU);
      }
      else {
        stage->stalled = 1;
      }
    }

    if (strcmp(stage->opcode, "LOAD") == 0) {
      if (allowed_dispatch(cpu, 1, 1, 0, 1)) {
        dispatch_instruction(cpu, 1, 1, 0, 1, 0, Int_FU);
      }
      else {
        stage->stalled = 1;
      }
    }

    if (strcmp(stage->opcode, "JUMP") == 0) {
      if (allowed_dispatch(cpu, 0, 0, 1, 1)) {
        dispatch_instruction(cpu, 0, 1, 0, 0, 1, Int_FU);
      }
      else {
        stage->stalled = 1;
      }
    }

    if (strcmp(stage->opcode, "JAL") == 0) {
      if (allowed_dispatch(cpu, 1, 0, 1, 1)) {
        dispatch_instruction(cpu, 1, 1, 0, 0, 1, Int_FU);
      }
      else {
        stage->stalled = 1;
      }
    }

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Decode/RF", cpu, DRF);
    }


    if (strcmp(stage->opcode, "HALT") == 0) {
      clear_stage(cpu, F);
      cpu->stage[F].busy = 1;
      stage->stalled = 1;
      if (allowed_dispatch(cpu, 0, 0, 0, 0)) {
        dispatch_instruction(cpu, 0, 0, 0, 0, 0, Int_FU);
        clear_stage(cpu, DRF);
      }
    }
    //printf("*** Decode: stalled=%d, busy=%d\n", stage->stalled, stage->busy);
  }
  else {
    if (stage->stalled) {
      if (strcmp(stage->opcode, "MOVC") == 0) {
        if (allowed_dispatch(cpu, 1, 0, 0, 1)) {
          dispatch_instruction(cpu, 1, 0, 0, 0, 0, Int_FU);
          stage->stalled = 0;
        }
      }

      if (strcmp(stage->opcode, "ADD") == 0 ||
          strcmp(stage->opcode, "SUB") == 0 ||
          strcmp(stage->opcode, "AND") == 0 ||
          strcmp(stage->opcode, "OR") == 0 ||
          strcmp(stage->opcode, "EX-OR") == 0) {

        if (allowed_dispatch(cpu, 1, 0, 0, 1)) {
          dispatch_instruction(cpu, 1, 1, 1, 0, 0, Int_FU);
          stage->stalled = 0;
        }
      }

      if (strcmp(stage->opcode, "ADDL") == 0 ||
          strcmp(stage->opcode, "SUBL") == 0) {
        if (allowed_dispatch(cpu, 1, 0, 0, 1)) {
          dispatch_instruction(cpu, 1, 1, 0, 0, 0, Int_FU);
          stage->stalled = 0;
        }
      }

      if (strcmp(stage->opcode, "MUL") == 0) {
        if (allowed_dispatch(cpu, 1, 0, 0, 1)) {
          dispatch_instruction(cpu, 1, 1, 1, 0, 0, Mul_FU);
          stage->stalled = 0;
        }
      }

      if (strcmp(stage->opcode, "BZ") == 0 ||
          strcmp(stage->opcode, "BNZ") == 0) {
        if (allowed_dispatch(cpu, 0, 0, 1, 1)) {
          dispatch_instruction(cpu, 0, 0, 0, 0, 1, Int_FU);
          stage->stalled = 0;
        }
      }

      if (strcmp(stage->opcode, "STORE") == 0) {
        if (allowed_dispatch(cpu, 0, 1, 0, 1)) {
          dispatch_instruction(cpu, 0, 1, 1, 1, 0, Int_FU);
          stage->stalled = 0;
        }
      }

      if (strcmp(stage->opcode, "LOAD") == 0) {
        if (allowed_dispatch(cpu, 1, 1, 0, 1)) {
          dispatch_instruction(cpu, 1, 1, 0, 1, 0, Int_FU);
          stage->stalled = 0;
        }
      }

      if (strcmp(stage->opcode, "JUMP") == 0) {
        if (allowed_dispatch(cpu, 0, 0, 1, 1)) {
          dispatch_instruction(cpu, 0, 1, 0, 0, 1, Int_FU);
          stage->stalled = 0;
        }
      }

      if (strcmp(stage->opcode, "JAL") == 0) {
        if (allowed_dispatch(cpu, 1, 0, 1, 1)) {
          dispatch_instruction(cpu, 1, 1, 0, 0, 1, Int_FU);
          stage->stalled = 0;
        }
      }

      if (strcmp(stage->opcode, "HALT") == 0) {
        if (allowed_dispatch(cpu, 0, 0, 0, 0)) {
          dispatch_instruction(cpu, 0, 0, 0, 0, 0, Int_FU);
        }
      }
    }

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Decode/RF", cpu, DRF);
    }


    if (strcmp(stage->opcode, "HALT") == 0) {
      if (allowed_dispatch(cpu, 0, 0, 0, 0)) {
        dispatch_instruction(cpu, 0, 0, 0, 0, 0, Int_FU);
        clear_stage(cpu, DRF);
      }
    }
    //printf("*** Decode: stalled=%d, busy=%d\n", stage->stalled, stage->busy);
  }
  return 0;
}

/*
 *  Execute Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
execute_int(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[Int_FU];
  if (!stage->busy && !stage->stalled) {

    if (strcmp(stage->opcode, "MOVC") == 0) {
      stage->buffer = stage->imm + 0;
    }

    if (strcmp(stage->opcode, "ADD") == 0) {
      stage->buffer = stage->rs1_value + stage->rs2_value;
    }

    if (strcmp(stage->opcode, "SUB") == 0) {
      stage->buffer = stage->rs1_value - stage->rs2_value;
    }

    if (strcmp(stage->opcode, "AND") == 0) {
      stage->buffer = stage->rs1_value & stage->rs2_value;
    }

    if (strcmp(stage->opcode, "OR") == 0) {
      stage->buffer = stage->rs1_value | stage->rs2_value;
    }

    if (strcmp(stage->opcode, "EX-OR") == 0) {
      stage->buffer = stage->rs1_value ^ stage->rs2_value;
    }

    if (strcmp(stage->opcode, "ADDL") == 0) {
      stage->buffer = stage->rs1_value + stage->imm;
    }

    if (strcmp(stage->opcode, "SUBL") == 0) {
      stage->buffer = stage->rs1_value - stage->imm;
    }

    if (strcmp(stage->opcode, "BZ") == 0) {
      int branch_id = stage->branch_id;
      int phys_src = cpu->bis.bis_entry[branch_id].phys_src;
      if (cpu->urf[phys_src].value == 0) {
        stage->target_address = stage->pc + stage->imm;
        control_flow(cpu);
      }
    }

    if (strcmp(stage->opcode, "BNZ") == 0) {
      int branch_id = stage->branch_id;
      int phys_src = cpu->bis.bis_entry[branch_id].phys_src;
      if (cpu->urf[phys_src].value != 0) {
        stage->target_address = stage->pc + stage->imm;
        control_flow(cpu);
      }
    }

    if (strcmp(stage->opcode, "JUMP") == 0) {
      stage->target_address = stage->rs1_value + stage->imm;
      control_flow(cpu);
    }

    if (strcmp(stage->opcode, "JAL") == 0) {
      stage->target_address = stage->rs1_value + stage->imm;
      stage->buffer = stage->pc + 4;
      printf("*** stage->target_address = %d\n", stage->target_address);
      printf("*** stage->buffer = %d\n", stage->buffer);
      control_flow(cpu);
    }

    if (strcmp(stage->opcode, "STORE") == 0) {
      stage->buffer = stage->rs2_value + stage->imm;
      if (stage->buffer > 4096 || stage->buffer < 0) {
        exception_handler(0, stage->opcode);
      }
      update_lsq_entry(cpu, Int_FU);
    }

    if (strcmp(stage->opcode, "LOAD") == 0) {
      stage->buffer = stage->rs1_value + stage->imm;
      if (stage->buffer > 4096 || stage->buffer < 0) {
        exception_handler(0, stage->opcode);
      }
      update_lsq_entry(cpu, Int_FU);
    }

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Execute_Int", cpu, Int_FU);
    }

    // Do not broadcast of those instructions that do not have physical destination address - BNZ, BZ, STORE
    if (strcmp(stage->opcode, "") != 0 && stage->phys_rd != -1 && (strcmp(stage->opcode, "LOAD") != 0)) {
      broadcast_result(cpu, Int_FU);
    }

    // Update for those instructions that are not in LSQ
    if (strcmp(stage->opcode, "") != 0 && stage->LSQ_index == -1) {
      update_rob_entry(cpu, Int_FU);
    }
    clear_stage(cpu, Int_FU);
  }
  else {
    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Execute_Int", cpu, Int_FU);
    }
  }
  return 0;
}

/*
 *  Execute Stage for MUL instruction
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
execute_mul(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[Mul_FU];
  if (!stage->busy && !stage->stalled) {

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Execute_Mul", cpu, Mul_FU);
    }

    if (strcmp(stage->opcode, "MUL") == 0) {
      stage->buffer = stage->rs1_value * stage->rs2_value;
      stage->stalled = 1;
      cpu->mul_cycle++;
    }

  }
  else {
    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Execute_Mul", cpu, Mul_FU);
    }

    if (strcmp(stage->opcode, "MUL") == 0) {
      stage->stalled = 0;
      broadcast_result(cpu, Mul_FU);
      update_rob_entry(cpu, Mul_FU);
      clear_stage(cpu, Mul_FU);
      cpu->mul_cycle = 1;
    }
  }
  return 0;
}

int
memory(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[MEM];
  if (!stage->busy && !stage->stalled) {
    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Memory", cpu, MEM);
    }

    if (strcmp(stage->opcode, "LOAD") == 0) {
      stage->buffer = cpu->data_memory[stage->mem_address];
      cpu->mem_cycle++;
      stage->stalled = 1;
    }

    if (strcmp(stage->opcode, "STORE") == 0) {
      cpu->mem_cycle++;
      stage->stalled = 1;
    }
  }
  else {
    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Memory", cpu, MEM);
    }

    if (stage->stalled) {

      if (cpu->mem_cycle == 3) {

        if (strcmp(stage->opcode, "STORE") == 0) {
          cpu->data_memory[stage->mem_address] = stage->rs1_value;
        }

        if (strcmp(stage->opcode, "LOAD") == 0) {
          broadcast_result(cpu, MEM);
          update_rob_entry(cpu, MEM);
        }
        stage->stalled = 0;
        cpu->mem_cycle = 1;
        clear_stage(cpu, MEM);
        return 0;
      }
      cpu->mem_cycle++;
    }
  }

  return 0;
}

int
APEX_cpu_run(APEX_CPU* cpu)
{
  while (cpu->clock <= cpu->code_memory_size) {
    /* All the instructions committed, so exit */
    if (cpu->simulation_completed) {
      printf("\n=============================== SIMULATION FINISHED ============================\n");
      break;
    }

    if (ENABLE_DEBUG_MESSAGES) {
      printf("\n================================ CLOCK CYCLE %d ================================\n\n", cpu->clock);
    }

    if (commit_rob_entry(cpu)) {
      commit_rob_entry(cpu);
    }
    memory(cpu);
    execute_int(cpu);
    execute_mul(cpu);
    if (ENABLE_DEBUG_MESSAGES) { display_iq(cpu); }
    process_iq(cpu);
    if (ENABLE_DEBUG_MESSAGES) {
      display_rob(cpu);
      display_lsq(cpu);
    }
    process_lsq(cpu);
    if (ENABLE_DEBUG_MESSAGES) { display_registers(cpu); }
    decode(cpu);
    fetch(cpu);

    cpu->clock++;
    cpu->fill_in_rob++;
    cpu->commitments = 0;
  }

  display_regs_mem(cpu);

  return 0;
}
