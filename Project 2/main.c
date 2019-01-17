/*
 *  main.c
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*#define IQ_ENTRIES_NUMBER 3
#define ROB_ENTRIES_NUMBER 3
#define URF_ENTRIES_NUMBER 10
#define RAT_ENTRIES_NUMBER 4
#define RRAT_ENTRIES_NUMBER 4
#define BIS_ENTRIES_NUMBER 3*/

#include "cpu.h"

int
main(int argc, char const* argv[])
{
  if (argc != 4) {
    fprintf(stderr, "APEX_Help : Usage %s <input_file>\n", argv[0]);
    exit(1);
  }

  int cycles = atoi(argv[3]);
  APEX_CPU* cpu = APEX_cpu_init(argv[1], argv[2], cycles);
  if (!cpu) {
    fprintf(stderr, "APEX_Error : Unable to initialize CPU\n");
    exit(1);
  }

  APEX_cpu_run(cpu);
  APEX_cpu_stop(cpu);
  return 0;
}
