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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

#define MAX_STR_LEN 32  // 定义缓冲区的最大长度

enum {
  TK_NOTYPE = 256,
  TK_EQ,
  TK_DNUM, 
  TK_UEQ,
  TK_AND,
  TK_NEQ,
  TK_HEXNUM,
  TK_DOLLAR,
  TK_UNARY_MINUS, // ‘-’ 作为取反 
  DEREF,
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"\\*", '*'},         // multiply
  {"/", '/'},           // devide
  {"-",'-'},            // minus  
  {"[0-9]+", TK_DNUM},  // 十进制nums
  {"\\(", '('},         // Left parenthesis
  {"\\)", ')'},         // Right parenthesis
  {"&&", TK_AND},       // logical AND
  {"!=", TK_NEQ},       // not equal
  {"0x[0-9a-fA-F]+", TK_HEXNUM}, // hexadecimal numbers starting with "0x"
  {"$", TK_DOLLAR},     // special token starting with "&"
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[MAX_STR_LEN];
} Token;

static Token tokens[1024] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0; // 已识别的标记数

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case '+' : 
            tokens[nr_token].type = '+';
            nr_token++;
            break;
          case '-' :
            tokens[nr_token].type = '-';
            nr_token++;
            break;
          case '*' :
            tokens[nr_token].type = '*';
            nr_token++;
            break;
          case '/' :
            tokens[nr_token].type = '/';
            nr_token++;
            break;
          case '(' :
            tokens[nr_token].type = '(';
            nr_token++;
            break;
          case ')' :
            tokens[nr_token].type = ')';
            nr_token++;
            break;                        
          case TK_DNUM :
            tokens[nr_token].type = TK_DNUM;
            if (substr_len < MAX_STR_LEN) {
              strncpy(tokens[nr_token].str, substr_start, substr_len);
              tokens[nr_token].str[substr_len] = '\0';
            } else {
              strncpy(tokens[nr_token].str, substr_start, MAX_STR_LEN - 1);
              tokens[nr_token].str[MAX_STR_LEN - 1] = '\0';
              printf("Warning: token too long, truncated to %s\n", tokens[nr_token].str);
              return false;
            }
            nr_token++;
            break;
          case TK_EQ :
            tokens[nr_token].type = TK_EQ;
            break;
          case TK_NOTYPE :
            break;
          default:
            printf("Unknown token type: %d\n", rules[i].token_type);
            return false;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

int get_priority(int type) {
  switch (type) {
    case '+':
    case '-':
      return 1; // 加法和减法的优先级为1
    case '*':
    case '/':
      return 2; // 乘法和除法的优先级为2
    // 根据需要添加其他运算符及其优先级
    default:
      return 20; // 默认优先级设为一个较大的值
  }
}



int find_main_operator(int p, int q) {
  int level = 0;
  int op = -1; // 主运算符的位置
  int min_priority = 20; // 初始化为一个较大的值

  for (int i = p; i <= q; i++) {
    int type = tokens[i].type;
    if (type == '(') {
      level++; // 遇到左括号，增加括号层次
    } else if (type == ')') {
      level--; // 遇到右括号，减少括号层次
    } else if (level == 0) { // 仅在括号外的运算符有效
      int priority = get_priority(type);
      if (priority <= min_priority) {
        min_priority = priority;
        op = i;
      }
    }
  }

  return op;
}



bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }
  int level = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') level++;
    if (tokens[i].type == ')') level--;
    if (level == 0 && i < q) return false;
  }
  return level == 0;
}

int eval(word_t p, word_t q) {   // eval的类型修改为int是为了避免运算中途递归调用时因为有负值存在而导致与c直接计算的答案不符
  printf("p= %u, q= %u \n", p, q);
  if (p > q) {
    printf("Bad expression\n");
    assert(0);
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if (tokens[p].type == TK_DNUM) {
      int val = atoi(tokens[p].str);
      // Check for preceding unary minus
      if (p > 0 && tokens[p - 1].type == TK_UNARY_MINUS) {
          val = -val;
      }
      return val;
    } else {
      printf("Unexpected token type\n");
      assert(0);
    }
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    int op = find_main_operator(p, q);

    // Apply unary minus before evaluating binary operations
    while (op > p && tokens[op - 1].type == TK_UNARY_MINUS) {
      int val = eval(p, op - 2);
      // Apply unary minus to the evaluated value
      val = -val;
      p = op - 1; // Update start position for next evaluation
      op = find_main_operator(p, q);
    }

    int val1 = eval(p, op - 1);
    int val2 = eval(op + 1, q);
    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      default: 
        printf("Unexpected operator\n");
        assert(0);
    }
  }  
  return 0;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  
  /* TODO: Insert codes to evaluate the expression. */

  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && (i == 0 || tokens[i - 1].type == '(' || tokens[i - 1].type == TK_EQ )) {   //这里type先乱写的
      tokens[i].type = DEREF;
    } 
    else if(tokens[i].type == '-') {
      if (i == 0 || tokens[i - 1].type == '(' || tokens[i - 1].type == '-' || tokens[i - 1].type == '+' ) {
        tokens[i].type = TK_UNARY_MINUS;
      }
    }
  }

  word_t result = (word_t)eval(0, nr_token - 1);
  *success = true;
  return result;
}
