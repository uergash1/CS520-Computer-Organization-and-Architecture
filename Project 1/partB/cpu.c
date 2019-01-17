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

/* Set this flag to 1 to enable debug messages */
int ENABLE_DEBUG_MESSAGES;

/* Flag to enable to display register and memory values */
int ENABLE_DISPLAY;

/* Flag to enable counting code_memory_size by the implemented logic */
int ENABLE_COUNTING;

/*
 * This function creates and initializes APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
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
    ENABLE_DISPLAY = 1;
  }
  else {
    ENABLE_DEBUG_MESSAGES = 1;
    ENABLE_DISPLAY = 1;
  }

  /* Initialize PC, Registers and all pipeline stages */
  cpu->pc = 4000;
  memset(cpu->regs, 0, sizeof(int) * 16);
  /*memset(cpu->regs_valid, 1, sizeof(int) * 16);*/
  memset(cpu->stage, 0, sizeof(CPU_Stage) * NUM_STAGES);
  memset(cpu->data_memory, 0, sizeof(int) * 4000);

  /* Parse input file and create code memory */
  cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);

  /* Making Z flag invalid for the first branch instruction */
  cpu->z_flag = 0;

  cpu->clock = 1;

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

  ENABLE_COUNTING = 0;
  cpu->code_memory_size = cycles;

  return cpu;
}

/*
 * This function de-allocates APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
 */
void
APEX_cpu_stop(APEX_CPU* cpu)
{
  free(cpu->code_memory);
  free(cpu);
}

/* Converts the PC(4000 series) into
 * array index for code memory
 *
 * Note : You are not supposed to edit this function
 *
 */
int
get_code_index(int pc)
{
  return (pc - 4000) / 4;
}

static void
print_instruction(CPU_Stage* stage)
{
  if (strcmp(stage->opcode, "STORE") == 0) {
    printf(
      "%s,R%d,R%d,#%d ", stage->opcode, stage->rs1, stage->rs2, stage->imm);
  }

  if (strcmp(stage->opcode, "LOAD") == 0) {
    printf(
      "%s,R%d,R%d,#%d ", stage->opcode, stage->rd, stage->rs1, stage->imm);
  }

  if (strcmp(stage->opcode, "MOVC") == 0) {
    printf("%s,R%d,#%d ", stage->opcode, stage->rd, stage->imm);
  }

  if (strcmp(stage->opcode, "ADD") == 0 ||
      strcmp(stage->opcode, "SUB") == 0 ||
      strcmp(stage->opcode, "AND") == 0 ||
      strcmp(stage->opcode, "OR") == 0 ||
      strcmp(stage->opcode, "EX-OR") == 0 ||
      strcmp(stage->opcode, "MUL") == 0) {

    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }

  if (strcmp(stage->opcode, "BZ") == 0 ||
      strcmp(stage->opcode, "BNZ") == 0) {
    printf("%s,#%d ", stage->opcode, stage->imm);
  }

  if (strcmp(stage->opcode, "JUMP") == 0) {
    printf("%s,R%d,#%d ", stage->opcode, stage->rs1, stage->imm);
  }

  if (strcmp(stage->opcode, "BUBBLE") == 0) {
    printf("%s", stage->opcode);
  }

  if (strcmp(stage->opcode, "HALT") == 0) {
    printf("%s", stage->opcode);
  }
}

/* Debug function which dumps the cpu stage
 * content
 *
 * Note : You are not supposed to edit this function
 *
 */
static void
print_stage_content(char* name, CPU_Stage* stage)
{
  printf("%-15s: pc(%d) ", name, stage->pc);
  print_instruction(stage);
  printf("\n");
}

/* Display function to display the content of reginsters and memory */
static void
display(APEX_CPU* cpu)
{
  printf("\n\n== STATE OF ARCHITECTURAL REGISTER FILE ==\n\n");
  for(int i = 0; i < 16; i++) {
    printf("|\tREG[%d]\t|\tValue = %d\t|\n", i, cpu->regs[i]);
  }

  printf("\n\n========== STATE OF DATA MEMORY ==========\n\n");
  for(int i = 0; i < 100; i++) {
    printf("|\tMEM[%d]\t|\tData Value = %d\t|\n", i, cpu->data_memory[i]);
  }
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

/* Control flow function for BZ, BNZ JUMP instructions
 * Takes branch and increases or decreases code_memory_size value if it is needed
 */
int
control_flow(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[EX];
  int difference = stage->buffer - stage->pc;

  strcpy(cpu->stage[DRF].opcode, "BUBBLE");
  cpu->stage[DRF].pc = 0000;

  strcpy(cpu->stage[F].opcode, "BUBBLE");
  cpu->stage[F].pc = 0000;
  cpu->stage[F].stalled = 1;

  cpu->pc = stage->buffer;

  if (ENABLE_COUNTING) {
    if (difference == 4) {
      cpu->code_memory_size += 2;
    }

    if (difference == 8) {
      cpu->code_memory_size++;
    }

    if (difference > 12) {
      int temp = (difference - 12) / 4;
      cpu->code_memory_size -= temp;
    }

    if (stage->imm < 0) {
      difference = abs(difference);
      int temp = difference / 4;
      cpu->code_memory_size = cpu->code_memory_size + temp + 3;
    }
  }
  return 0;
}

/* Logic for getting values for source registers.
 * Some instructions does NOT have source register 2. In this case, rs2_exist should be 0.
 * The only cases when Decode/RF stage stalls is when there is LOAD instruction in the next stage.
 */
int
get_source_values(APEX_CPU* cpu, int rs2_exist)
{
  CPU_Stage* stage = &cpu->stage[DRF];

  /* Resolving dependency of source register 1 */
  /* Checking whether rs1 is used in EX stage as rd */
  /* If EX stage contains one of following instructions (BUBBLE, STORE, BZ, BNZ, JUMP), then we do NOT care of its rd */
  int rs1_found = 0;
  if (stage->rs1 == cpu->stage[EX].rd &&
      strcmp(cpu->stage[EX].opcode, "BUBBLE") != 0 &&
      strcmp(cpu->stage[EX].opcode, "STORE") != 0 &&
      strcmp(cpu->stage[EX].opcode, "BZ") != 0 &&
      strcmp(cpu->stage[EX].opcode, "BNZ") != 0 &&
      strcmp(cpu->stage[EX].opcode, "JUMP") != 0) {

    rs1_found = 1;
    if (strcmp(cpu->stage[EX].opcode, "LOAD") != 0) {
      stage->rs1_value = cpu->stage[EX].buffer;
      printf("1ex-nl,");
    }
    else {
      printf("1ex-l,");
      stage->stalled = 1;
    }
  }

  /* Checking whether rs1 is used in MEM stage as rd */
  /* If MEM stage contains one of following instructions (BUBBLE, STORE, BZ, BNZ, JUMP), then we do NOT care of its rd */
  if (!rs1_found &&
      stage->rs1 == cpu->stage[WB].rd &&
      strcmp(cpu->stage[WB].opcode, "BUBBLE") != 0 &&
      strcmp(cpu->stage[WB].opcode, "STORE") != 00 &&
      strcmp(cpu->stage[WB].opcode, "BZ") != 0 &&
      strcmp(cpu->stage[WB].opcode, "BNZ") != 0 &&
      strcmp(cpu->stage[WB].opcode, "JUMP") != 0) {

    stage->rs1_value = cpu->stage[WB].buffer;
    rs1_found = 1;
    printf("1mem,");
  }

  /* If rs1 is not found in EX and MEM stages, then the most recent value of rs1 is in register file */
  if (!rs1_found) {
    printf("1rf,");
    stage->rs1_value = cpu->regs[stage->rs1];
  }
  printf("r%d=%d,", stage->rs1, stage->rs1_value);

  /* Resolving dependency of source register 2 */
  /* Checking whether rs2 is used in EX stage as rd */
  /* If EX stage contains one of following instructions (BUBBLE, STORE, BZ, BNZ, JUMP), then we do NOT care of its rd */
  if (rs2_exist) {
    int rs2_found = 0;
    if (stage->rs2 == cpu->stage[EX].rd &&
        strcmp(cpu->stage[EX].opcode, "BUBBLE") != 0 &&
        strcmp(cpu->stage[EX].opcode, "STORE") != 00 &&
        strcmp(cpu->stage[EX].opcode, "BZ") != 0 &&
        strcmp(cpu->stage[EX].opcode, "BNZ") != 0 &&
        strcmp(cpu->stage[EX].opcode, "JUMP") != 0) {

      rs2_found = 1;
      if (strcmp(cpu->stage[EX].opcode, "LOAD") != 0) {
        stage->rs2_value = cpu->stage[EX].buffer;
        printf("2ex-nl,");
      }
      else {
        stage->stalled = 1;
        printf("2ex-l,");
      }
    }

    /* Checking whether rs2 is used in MEM stage as rd */
    /* If MEM stage contains one of following instructions (BUBBLE, STORE, BZ, BNZ, JUMP), then we do NOT care of its rd */
    if (!rs2_found &&
        stage->rs2 == cpu->stage[WB].rd &&
        strcmp(cpu->stage[WB].opcode, "BUBBLE") != 0 &&
        strcmp(cpu->stage[WB].opcode, "STORE") != 00 &&
        strcmp(cpu->stage[WB].opcode, "BZ") != 0 &&
        strcmp(cpu->stage[WB].opcode, "BNZ") != 0 &&
        strcmp(cpu->stage[WB].opcode, "JUMP") != 0) {

      stage->rs2_value = cpu->stage[WB].buffer;
      rs2_found = 1;
      printf("2mem,");
    }

    /* If rs2 is not found in EX and MEM stages, then the most recent value of rs2 is in register file */
    if (!rs2_found) {
      printf("2rf,");
      stage->rs2_value = cpu->regs[stage->rs2];
    }
    printf("r%d=%d,", stage->rs2, stage->rs2_value);
  }
  return 0;
}

/*
 *  Fetch Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
fetch(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[F];
  if (!stage->busy && !stage->stalled) {

    /* Store current PC in fetch latch */
    stage->pc = cpu->pc;

    /* Index into code memory using this pc and copy all instruction fields into
     * fetch latch
     */
    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
    strcpy(stage->opcode, current_ins->opcode);
    stage->rd = current_ins->rd;
    stage->rs1 = current_ins->rs1;
    stage->rs2 = current_ins->rs2;
    stage->imm = current_ins->imm;
    stage->rd = current_ins->rd;
    stage->z_flag = cpu->z_flag;

    /* Update PC for next instruction */
    cpu->pc += 4;

    /* Copy data from fetch latch to decode latch */
    if (!cpu->stage[DRF].stalled) {
      cpu->stage[DRF] = cpu->stage[F];
    }
    else {
      stage->stalled = 1;
    }

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Fetch", stage);
    }
  }
  else {
    /* If HALT reached into WB stage, finish program  */
    if (strcmp(cpu->stage[WB].opcode, "HALT") == 0) {
      cpu->code_memory_size = cpu->clock + 1;
    }

    /* If current stage contains BUBBLE, then stop stalling.
     * BZ and BNZ can introduce BUBBLE into fetch stage
     */
    if (strcmp(stage->opcode, "BUBBLE") == 0) {
      stage->stalled = 0;
      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Fetch", stage);
      }
    }

    /* Copy data from fetch latch to decode latch
     * if DRF is not stalled
     */
    if (!cpu->stage[DRF].stalled && strcmp(cpu->stage[DRF].opcode, "BUBBLE") != 0) {
      stage->stalled = 0;
      cpu->stage[DRF] = cpu->stage[F];
      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Fetch", stage);
      }
    }

    /* Show content of fetch stage if current stage does not contain HALT */
    if (cpu->stage[DRF].stalled && strcmp(cpu->stage[DRF].opcode, "HALT") != 0) {
      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Fetch", stage);
      }
    }
  }
  return 0;
}

/*
 *  Decode Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
decode(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[DRF];
  if (!stage->busy && !stage->stalled) {

    /* Read data from register file for store */
    if (strcmp(stage->opcode, "STORE") == 0) {
      get_source_values(cpu, 1);
    }

    /* Read data from register file for LOAD */
    if (strcmp(stage->opcode, "LOAD") == 0) {
      get_source_values(cpu, 0);
    }

    /* No Register file read needed for MOVC */
    if (strcmp(stage->opcode, "MOVC") == 0) {
      /* Nothing has to be done */
    }

    /* Read data from register file for ADD, SUB, AND, OR, EX-OR, MUL */
    if (strcmp(stage->opcode, "ADD") == 0 ||
        strcmp(stage->opcode, "SUB") == 0 ||
        strcmp(stage->opcode, "AND") == 0 ||
        strcmp(stage->opcode, "OR") == 0 ||
        strcmp(stage->opcode, "EX-OR") == 0 ||
        strcmp(stage->opcode, "MUL") == 0) {

      get_source_values(cpu, 1);
    }

    /* Check whether Z_flag is valid for BZ and BNZ */
    if (strcmp(stage->opcode, "BZ") == 0 ||
        strcmp(stage->opcode, "BNZ") == 0) {

      /* Branch instruction checks whether next (EX) stage contains ADD or SUB or MUL */
      int ins_found = 0;
      if (strcmp(cpu->stage[EX].opcode, "ADD") == 0 ||
          strcmp(cpu->stage[EX].opcode, "SUB") == 0 ||
          strcmp(cpu->stage[EX].opcode, "MUL") == 0) {

        /* It gets buffer of EX stage and checks whether it is ZERO */
        ins_found = 1;
        if (cpu->stage[EX].buffer == 0) {
          stage->z_flag = 1;
        }
        else {
          stage->z_flag = 0;
        }
      }

      /* If it could NOT find in EX stage, then
       * branch instruction checks whether MEM stage contains ADD or SUB or MUL
       */
      if (!ins_found &&
          (strcmp(cpu->stage[WB].opcode, "ADD") == 0 ||
          strcmp(cpu->stage[WB].opcode, "SUB") == 0 ||
          strcmp(cpu->stage[WB].opcode, "MUL") == 0)) {

        /* It gets buffer of EX stage and checks whether it is ZERO */
        if (cpu->stage[WB].buffer == 0) {
          stage->z_flag = 1;
        }
        else {
          stage->z_flag = 0;
        }
      }
    }

    /* Read data from register file for JUMP */
    if (strcmp(stage->opcode, "JUMP") == 0) {
      get_source_values(cpu, 0);
    }

    /* Copy data from decode latch to execute latch
       if decode stage is not stalled
       else transfer bubble*/
    if (!stage->stalled) {
      cpu->stage[EX] = cpu->stage[DRF];
    }
    else {
      strcpy(cpu->stage[EX].opcode, "BUBBLE");
      cpu->stage[EX].pc = 0000;
    }

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Decode/RF", stage);
    }

    /* If current stage contains HALT, then display it once, and stall Fetch and Decode stages */
    if (strcmp(stage->opcode, "HALT") == 0) {
      cpu->stage[F].stalled = 1;
      stage->stalled = 1;
    }
  }
  else {

    /* If current stage does not contain HALT and if EX stage is not stalled,
     * then current stage is stalled because of dependency between source in current stage
     * and destination registers in EX stage
     */
    if (stage->stalled && strcmp(stage->opcode, "HALT") != 0 && !cpu->stage[EX].stalled) {
      if (ENABLE_COUNTING) {
        cpu->code_memory_size++;
      }

      if (strcmp(stage->opcode, "ADD") == 0 ||
          strcmp(stage->opcode, "SUB") == 0 ||
          strcmp(stage->opcode, "AND") == 0 ||
          strcmp(stage->opcode, "OR") == 0 ||
          strcmp(stage->opcode, "EX-OR") == 0 ||
          strcmp(stage->opcode, "MUL") == 0) {

        get_source_values(cpu, 1);
        stage->stalled = 0;
        cpu->stage[EX] = cpu->stage[DRF];
      }

      if (strcmp(stage->opcode, "LOAD") == 0) {
        get_source_values(cpu, 0);
        stage->stalled = 0;
        cpu->stage[EX] = cpu->stage[DRF];
      }

      if (strcmp(stage->opcode, "JUMP") == 0) {
        get_source_values(cpu, 0);
        stage->stalled = 0;
        cpu->stage[EX] = cpu->stage[DRF];
      }

      if (strcmp(stage->opcode, "STORE") == 0) {
        get_source_values(cpu, 1);
        stage->stalled = 0;
        cpu->stage[EX] = cpu->stage[DRF];
      }

      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Decode/RF", stage);
      }
    }

    if (cpu->stage[EX].stalled && strcmp(cpu->stage[EX].opcode, "HALT") != 0) {
      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Decode/RF", stage);
      }
    }
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
execute(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[EX];
  if (!stage->busy && !stage->stalled) {

    /* STORE */
    if (strcmp(stage->opcode, "STORE") == 0) {
      stage->mem_address = stage->rs2_value + stage->imm;
      if (stage->mem_address > 4096 || stage->mem_address < 0) {
        exception_handler(0, stage->opcode);
      }
      printf("mem=%d,", stage->mem_address);
    }

     /* LOAD */
    if (strcmp(stage->opcode, "LOAD") == 0) {
      stage->mem_address = stage->rs1_value + stage->imm;
      if (stage->mem_address > 4096 || stage->mem_address < 0) {
        exception_handler(0, stage->opcode);
      }
      printf("mem=%d,", stage->mem_address);
    }

    /* MOVC */
    if (strcmp(stage->opcode, "MOVC") == 0) {
      stage->buffer = stage->imm + 0;
    }

    /* ADD */
    if (strcmp(stage->opcode, "ADD") == 0) {
      stage->buffer = stage->rs1_value + stage->rs2_value;
    }

    /* SUB */
    if (strcmp(stage->opcode, "SUB") == 0) {
      stage->buffer = stage->rs1_value - stage->rs2_value;
    }

    /* MUL - stall for one cycle */
    if (strcmp(stage->opcode, "MUL") == 0) {
      stage->buffer = stage->rs1_value * stage->rs2_value;
      stage->stalled = 1;
    }

    /* AND */
    if (strcmp(stage->opcode, "AND") == 0) {
      stage->buffer = stage->rs1_value & stage->rs2_value;
    }

    /* OR */
    if (strcmp(stage->opcode, "OR") == 0) {
      stage->buffer = stage->rs1_value | stage->rs2_value;
    }

    /* EX-OR */
    if (strcmp(stage->opcode, "EX-OR") == 0) {
      stage->buffer = stage->rs1_value ^ stage->rs2_value;
    }

    if (strcmp(stage->opcode, "JUMP") == 0) {
      stage->buffer = stage->rs1_value + stage->imm;
      control_flow(cpu);
    }

    if (strcmp(stage->opcode, "BNZ") == 0) {
      if (!stage->z_flag) {
        stage->buffer = stage->pc + stage->imm;
        control_flow(cpu);
      }
      printf("z_flag=%d,", stage->z_flag);
    }

    if (strcmp(stage->opcode, "BZ") == 0) {
      if (stage->z_flag) {
        stage->buffer = stage->pc + stage->imm;
        control_flow(cpu);
      }
      printf("z_flag=%d,", stage->z_flag);
    }

    printf("r%d=%d,r%d=%d,d%d=%d,", stage->rs1, stage->rs1_value, stage->rs2, stage->rs2_value, stage->rd, stage->buffer);

    /* If it is stalled, do not copy data into the next stage, and introduce BUBBLE into MEM stage
     * EX stage can be stalled only by MUL instruction
     */
    if (!stage->stalled) {
      cpu->stage[MEM] = cpu->stage[EX];
    }
    else {
      cpu->stage[DRF].stalled = 1;
      strcpy(cpu->stage[MEM].opcode, "BUBBLE");
      cpu->stage[MEM].pc = 0000;
    }

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Execute", stage);
    }

    /* If current stage contains HALT, then show once and stall the stage */
    if (strcmp(stage->opcode, "HALT") == 0) {
      stage->stalled = 1;
    }
  }
  else {
    /* If it is stalled by MUL instruction
     * stop stalling and increase code memory size by one
     */
    if (stage->stalled && strcmp(stage->opcode, "MUL") == 0) {
      if (ENABLE_COUNTING) {
        cpu->code_memory_size++;
      }
      stage->stalled = 0;
      cpu->stage[DRF].stalled = 0;
      cpu->stage[MEM] = cpu->stage[EX];

      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Execute", stage);
      }
    }
  }
  return 0;
}

/*
 *  Memory Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
memory(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[MEM];
  if (!stage->busy && !stage->stalled) {

    /* STORE */
    if (strcmp(stage->opcode, "STORE") == 0) {
      cpu->data_memory[stage->mem_address] = stage->rs1_value;
      printf("mad=%d,data=%d,", stage->mem_address, cpu->data_memory[stage->mem_address]);
    }

    /* LOAD */
    if (strcmp(stage->opcode, "LOAD") == 0) {
      stage->buffer = cpu->data_memory[stage->mem_address];
      printf("mad=%d,data=%d,", stage->mem_address, stage->buffer);
    }

    printf("r%d=%d,r%d=%d,d%d=%d,", stage->rs1, stage->rs1_value, stage->rs2, stage->rs2_value, stage->rd, stage->buffer);

    /* Copy data from decode latch to execute latch*/
    cpu->stage[WB] = cpu->stage[MEM];

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Memory", stage);
    }

    if (strcmp(stage->opcode, "HALT") == 0) {
      stage->stalled = 1;
    }
  }

  return 0;
}

/*
 *  Writeback Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
writeback(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[WB];
  if (!stage->busy && !stage->stalled) {

    /* Update register file */
    if (strcmp(stage->opcode, "MOVC") == 0 ||
        strcmp(stage->opcode, "SUB") == 0 ||
        strcmp(stage->opcode, "ADD") == 0 ||
        strcmp(stage->opcode, "AND") == 0 ||
        strcmp(stage->opcode, "OR") == 0 ||
        strcmp(stage->opcode, "EX-OR") == 0 ||
        strcmp(stage->opcode, "LOAD") == 0 ||
        strcmp(stage->opcode, "MUL") == 0) {

      cpu->regs[stage->rd] = stage->buffer;
      /*printf("%d", cpu->regs[stage->rd]);*/
    }

    /* Update Z-flag */
    if (strcmp(stage->opcode, "SUB") == 0 ||
        strcmp(stage->opcode, "ADD") == 0 ||
        strcmp(stage->opcode, "MUL") == 0) {

      if (stage->buffer == 0) {
        cpu->z_flag = 1;
      }
      else {
        cpu->z_flag = 0;
      }
    }

    printf("r%d=%d,r%d=%d,d%d=%d,", stage->rs1, stage->rs1_value, stage->rs2, stage->rs2_value, stage->rd, stage->buffer);

    cpu->ins_completed++;

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Writeback", stage);
    }

    if (strcmp(stage->opcode, "HALT") == 0) {
      strcpy(stage->opcode, "");
    }
  }
  return 0;
}

/*
 *  APEX CPU simulation loop
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
APEX_cpu_run(APEX_CPU* cpu)
{
  while (cpu->clock <= cpu->code_memory_size) {
    if (ENABLE_COUNTING) {
      /* All the instructions committed, so exit */
      if (cpu->ins_completed == cpu->code_memory_size) {
        printf("(apex) >> Simulation Complete\n");
        break;
      }
    }

    if (ENABLE_DEBUG_MESSAGES) {
      printf("--------------------------------\n");
      printf("Clock Cycle #: %d\n", cpu->clock);
      printf("--------------------------------\n");
    }

    writeback(cpu);
    memory(cpu);
    execute(cpu);
    decode(cpu);
    fetch(cpu);
    cpu->clock++;
  }

  if (ENABLE_DISPLAY) {
    display(cpu);
  }

  return 0;
}
