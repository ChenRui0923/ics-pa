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
#include "sdb.h"
#include "memory/vaddr.h"

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
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char *args) {
  int n = 1;
  if (args != NULL) {
    n = atoi(args);
    if (n <= 0) {
      printf("Invalid argument for si: %s\n", args);
      return 0;
    }
  }
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {
   if (args != NULL) {
    if (strcmp(args, "r") == 0) {
      isa_reg_display();
    } else if (strcmp(args, "w" )== 0) {
      printf("w---this is a test for print monitoring points \n");
    } else {
      printf("Invalid argument for info: %s\n", args);
    }
  } else {
    printf("Please input your arugument for info \n");
  }
  return 0;
}


// Check if the string is a hex num
bool is_hex(const char *str) {
  if (str == NULL) {
    return false;
  }

  // Check if the string starts with "0x" or "0X"
  if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
    str += 2; // Skip the "0x" prefix

    // Check if the rest of the string contains only valid hex characters
    while (*str) {
      if (!isxdigit(*str)) {
        return false;
      }
      str++;
    }
    return true;
  }
  return false;
}


static int cmd_x(char *args) {
  char* n_str = strtok(args, " ");
  char* expr_str = strtok(NULL, " ");
  if (n_str == NULL || expr_str == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  int n = atoi(n_str);
  if (n <= 0) {
    printf("Invalid argument for x: %s\n", n_str);
    return 0;
  }
  if(!is_hex(expr_str)) {
    printf("Invalid argument for x: %s\n", expr_str);
    return 0;
  }
  
  vaddr_t addr = strtoul(expr_str, NULL, 16);
  if (addr < 0x80000000 || addr + n * 4 > 0x88000000) {
    printf("Invalid address. Please input adress in bound of pmem [0x80000000, 0x87ffffff] \n");
    return 0;
  }
  for (int i = 0; i < n; i++) {
    printf("0x%08x\n", vaddr_read(addr, 4));
    addr += 4;
  }

  return 0;
}

static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }
  bool flag = true;
  word_t result = expr(args, &flag);
  if (flag) {
    printf("%u\n", (uint32_t)result);
  } else {
    printf("Failed to evaluate expression: %s\n", args);
  }
  return result;
}

static int cmd_w(char *args) {
  return 0;
}

static int cmd_d(char *args) {
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Single Step", cmd_si},
  { "info", "Printing program information", cmd_info},
  { "x", "Scan Memory ", cmd_x},
  { "p", "Expression evaluation", cmd_p },
  { "w", "Set up monitoring points", cmd_w},
  { "d", "Deleting a Watchpoint", cmd_d },


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
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
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
