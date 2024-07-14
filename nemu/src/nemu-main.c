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

#include <common.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

word_t expr(char *e, bool *success);

void run_tests(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    perror("fopen");
    printf("Attempted to open file: %s\n", filename);
    return;
  }

  char line[1024];
  while (fgets(line, sizeof(line), fp)) {
    // 去除换行符和回车符
    line[strcspn(line, "\n")] = '\0';
    line[strcspn(line, "\r")] = '\0';

    char *expr_str = strchr(line, ' ');
    if (!expr_str) continue;

    *expr_str = '\0';  // Split the line into result and expression
    expr_str++;

    int expected_result = atoi(line);
    bool success = true;
    word_t result = expr(expr_str, &success);

    // printf("Evaluating: %s\n", expr_str);
    if (success) {
      if (result == expected_result) {
        printf("PASS: %s = %u\n", expr_str, result);
      } else {
        printf("FAIL: %s = %u (expected %u)\n", expr_str, result, expected_result);
      }
    } else {
      printf("ERROR: Failed to evaluate expression: %s\n", expr_str);
    }
  }

  fclose(fp);
}


int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  const char *input_file = "/home/gpu04/ray_os/pa/ics2023/nemu/tools/gen-expr/input";
  printf("Reading from input file: %s\n", input_file);
  run_tests(input_file);

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}
