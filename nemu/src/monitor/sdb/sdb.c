/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <memory/paddr.h>
#include "sdb.h"
#include "watchpoint.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_END;
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
	char *arg = strtok(NULL, " ");
	unsigned step;
	if(arg == NULL)step = 1;
	else step = (unsigned)atoi(arg);
	
	cpu_exec(step);
	return 0;
}

static int cmd_info(char *args) {
	char *arg = strtok(NULL, " ");
	if(arg == NULL) {
		printf("info what? register or watch point?\n");
	}
	else if(arg[0] == 'r') {
		isa_reg_display();
	}
	else if(arg[0] == 'w') {
		print_all_wps();
	}
	return 0;
}

static int cmd_x(char *args) {
	char *arg1 = strtok(NULL, " ");
	char *arg2 = strtok(NULL, " ");
	int N = 0;
	vaddr_t addr = 0;

	if(arg1 == NULL || arg2 == NULL){
		printf("invalid input!\n");
	}
	else {
		N = (vaddr_t)atoi(arg1);
    addr = (vaddr_t)strtoll(arg2, NULL, 16);
		//printf("the input value is %u \t %x \n", N, addr);
		for(int i = 0; i < N; i++,addr++){
			printf("the value of 0x%lu is 0x%lu \n", addr, paddr_read(addr, 8));
		}
	}

	return 0;
}

static int cmd_p(char *args) {
	bool is_success = true;
	long unsigned result = expr(args, &is_success);
	Log(" %s  result is %lu ", args, result);

	return 0;
}

static int cmd_watch(char *args) {
	char *arg1 = strtok(NULL, " ");
	bool is_success = false;
	word_t val = expr(arg1, &is_success);
	if(is_success){
		WP *p = new_wp();
		memcpy(p->exp, arg1, sizeof(char) * strlen(arg1));
		p->exp_val = val;
		printf("the watchpoint %d set at %s (value = %lu)\n", p->NO, p->exp, p->exp_val);	
	}
	else {
		printf("Invalid Expression!!!\n");
	}
	return 0;
}

static int cmd_d(char *args) {
	char *arg1 = strtok(NULL, " ");
	int n = atoi(arg1);
	delete_wp(n);
	return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
	{"si", "Single step", cmd_si },
	{"info", "Print the status of program", cmd_info },
	{"x", "Scan the memery", cmd_x },
  {"p", "calculate the expression", cmd_p },
	{"watch", "add watch point", cmd_watch},
	{"d", "delete the watch point", cmd_d}
 	/* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
     }
   }
  else { 
    for ( i = 0; i < NR_CMD; i ++) {
      if ( strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
