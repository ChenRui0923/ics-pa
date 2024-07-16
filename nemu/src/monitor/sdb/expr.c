// /***************************************************************************************
// * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
// *
// * NEMU is licensed under Mulan PSL v2.
// * You can use this software according to the terms and conditions of the Mulan PSL v2.
// * You may obtain a copy of Mulan PSL v2 at:
// *          http://license.coscl.org.cn/MulanPSL2
// *
// * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
// *
// * See the Mulan PSL v2 for more details.
// ***************************************************************************************/

// #include <isa.h>

// /* We use the POSIX regex functions to process regular expressions.
//  * Type 'man regex' for more information about POSIX regex functions.
//  */
// #include <regex.h>

// #define MAX_STR_LEN 32  // 定义缓冲区的最大长度

// enum {
//   TK_NOTYPE = 256,
//   TK_EQ,
//   TK_DNUM, 
//   TK_AND,
//   TK_NEQ,
//   TK_HEXNUM,
//   TK_DOLLAR,
//   TK_UNARY_MINUS, // ‘-’ 作为取反 
//   DEREF,
//   /* TODO: Add more token types */

// };

// static struct rule {
//   const char *regex;
//   int token_type;
// } rules[] = {

//   /* TODO: Add more rules.
//    * Pay attention to the precedence level of different rules.
//    */

//   {" +", TK_NOTYPE},    // spaces
//   {"\\+", '+'},         // plus
//   {"==", TK_EQ},        // equal
//   {"\\*", '*'},         // multiply
//   {"/", '/'},           // devide
//   {"-",'-'},            // minus  
//   {"[0-9]+", TK_DNUM},  // 十进制nums
//   {"\\(", '('},         // Left parenthesis
//   {"\\)", ')'},         // Right parenthesis
//   {"&&", TK_AND},       // logical AND
//   {"!=", TK_NEQ},       // not equal
//   {"0x[0-9a-fA-F]+", TK_HEXNUM}, // hexadecimal numbers starting with "0x"
//   {"$", TK_DOLLAR},     // special token starting with "&"
// };

// #define NR_REGEX ARRLEN(rules)

// static regex_t re[NR_REGEX] = {};

// /* Rules are used for many times.
//  * Therefore we compile them only once before any usage.
//  */
// void init_regex() {
//   int i;
//   char error_msg[128];
//   int ret;

//   for (i = 0; i < NR_REGEX; i ++) {
//     ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
//     if (ret != 0) {
//       regerror(ret, &re[i], error_msg, 128);
//       panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
//     }
//   }
// }

// typedef struct token {
//   int type;
//   char str[MAX_STR_LEN];
// } Token;

// static Token tokens[500] __attribute__((used)) = {};
// static int nr_token __attribute__((used))  = 0;

// static bool make_token(char *e) {
//   int position = 0;
//   int i;
//   regmatch_t pmatch;

//   nr_token = 0; // 已识别的标记数

//   while (e[position] != '\0') {
//     /* Try all rules one by one. */
//     for (i = 0; i < NR_REGEX; i ++) {
//       if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
//         char *substr_start = e + position;
//         int substr_len = pmatch.rm_eo;

//         Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
//             i, rules[i].regex, position, substr_len, substr_len, substr_start);

//         position += substr_len;

//         /* TODO: Now a new token is recognized with rules[i]. Add codes
//          * to record the token in the array `tokens'. For certain types
//          * of tokens, some extra actions should be performed.
//          */

//         switch (rules[i].token_type) {
//           case 256: break;
//           default: {
//             tokens[nr_token].type=rules[i].token_type;
//             strncpy(tokens[nr_token].str,substr_start,substr_len);
//             nr_token++;
//             break;
//           }
//         }
//         break;
//       }
//     }
//     if (i == NR_REGEX) {
//       printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
//       return false;
//     }
//   }

//   return true;
// }

// int get_priority(int type) {
//   switch (type) {
//     case '+':
//     case '-':
//       return 1; // 加法和减法的优先级为1
//     case '*':
//     case '/':
//       return 2; // 乘法和除法的优先级为2
//     // 根据需要添加其他运算符及其优先级
//     default:
//       return 20; // 默认优先级设为一个较大的值
//   }
// }



// int find_main_operator(int p, int q) {
//   int level = 0;
//   int op = -1; // 主运算符的位置
//   int min_priority = 20; // 初始化为一个较大的值

//   for (int i = p; i <= q; i++) {
//     int type = tokens[i].type;
//     if (type == '(') {
//       level++; // 遇到左括号，增加括号层次
//     } else if (type == ')') {
//       level--; // 遇到右括号，减少括号层次
//     } else if (level == 0) { // 仅在括号外的运算符有效
//       int priority = get_priority(type);
//       if (priority <= min_priority) {
//         min_priority = priority;
//         op = i;
//       }
//     }
//   }

//   return op;
// }



// bool check_parentheses(int p, int q) {
//   if (tokens[p].type != '(' || tokens[q].type != ')') {
//     return false;
//   }
//   int level = 0;
//   for (int i = p; i <= q; i++) {
//     if (tokens[i].type == '(') level++;
//     if (tokens[i].type == ')') level--;
//     if (level == 0 && i < q) return false;
//   }
//   return level == 0;
// }

// int eval(word_t p, word_t q) {   // eval的类型修改为int是为了避免运算中途递归调用时因为有负值存在而导致与c直接计算的答案不符
//   printf("p= %u, q= %u \n", p, q);
//   if (p > q) {
//     printf("Bad expression\n");
//     assert(0);
//   }
//   else if (p == q) {
//     /* Single token.
//      * For now this token should be a number.
//      * Return the value of the number.
//      */
//     if (tokens[p].type == TK_DNUM) {
//       int val = atoi(tokens[p].str);
//       // Check for preceding unary minus
//       if (p > 0 && tokens[p - 1].type == TK_UNARY_MINUS) {
//           val = -val;
//       }
//       return val;
//     } else {
//       printf("Unexpected token type\n");
//       // assert(0);
//     }
//   }
//   else if (check_parentheses(p, q) == true) {
//     /* The expression is surrounded by a matched pair of parentheses.
//      * If that is the case, just throw away the parentheses.
//      */
//     return eval(p + 1, q - 1);
//   }
//   else {
//     int op = find_main_operator(p, q);

//     int val1 = 0;
//     if (tokens[op].type == TK_UNARY_MINUS) {
//       val1 = eval(op + 1, q);
//       return -val1;
//     } else {
//       val1 = eval(p, op - 1);
//       int val2 = eval(op + 1, q);
//       switch (tokens[op].type) {
//         case '+': return val1 + val2;
//         case '-': return val1 - val2;
//         case '*': return val1 * val2;
//         case '/': return val1 / val2;
//         default:
//           printf("Unexpected operator\n");
//           assert(0);
//       }
//     }
//   } 
//   return 0;
// }


// word_t expr(char *e, bool *success) {
//   if (!make_token(e)) {
//     *success = false;
//     return 0;
//   }

  
//   /* TODO: Insert codes to evaluate the expression. */

//   for (int i = 0; i < nr_token; i ++) {
//     // dereference
//     if (tokens[i].type == '*' && (i == 0 || tokens[i - 1].type == '(' || tokens[i - 1].type == TK_EQ )) {   //这里type先乱写的
//       tokens[i].type = DEREF;
//     } 
//     // negative
//     else if(tokens[i].type=='-'&&(i==0||(tokens[i-1].type!= TK_DNUM &&tokens[i-1].type!=')'))){
//       tokens[i].type=TK_UNARY_MINUS;
//       tokens[i].str[0]='-';
//     }
//   }

//   word_t result = (word_t)eval(0, nr_token - 1);
//   *success = true;
//   return result;
// }




#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

#include "memory/paddr.h"

enum {
  TK_NOTYPE = 256, TK_EQ=255, NUMBER=254,negative=253,
  sixteen=252,reg=251,deref=250,and=249,neq=248,PC=257,

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {"0x[0-9a-f]+",sixteen},
  {" +", TK_NOTYPE},    //spaces
  {"\\+", '+'},         //plus
  {"==", TK_EQ},        //equal
  {"\\-", '-'},         //sub
  {"\\*",'*'},          //mul
  {"/",'/'},            //div
  {"[0-9]+",NUMBER},    
  {"\\(",'('},          
  {"\\)",')'},          
  {"\\$\\p\\c",PC},
  {"\\$[a-z]+",reg},
  {"&&",and},
  {"!=",neq},
  
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX] = {};

unsigned int calculate();

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
  char str[32];
} Token;

static Token tokens[500] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;
  memset(tokens,0,sizeof(tokens));

  while (e[position] != '\0' && e[position]!='\n') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position; 
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;
        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        switch (rules[i].token_type) {
          case 256: break;
          default: {
            tokens[nr_token].type=rules[i].token_type;
            strncpy(tokens[nr_token].str,substr_start,substr_len);
            nr_token++;
            break;
          }
        }
        break;
      }
    }
    // something wrong happend
    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


word_t expr(char *e, bool *if_success) {
  if (!make_token(e)) {
    *if_success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  for(int i = 0;i < nr_token;i++){
    //negative number
    if(tokens[i].type=='-'&&(i==0||(tokens[i-1].type!=NUMBER&&tokens[i-1].type!=')'))){
      tokens[i].type=negative;
      tokens[i].str[0]='-';
    }
    //dereference
    if(tokens[i].type=='*'&& (i == 0 || (tokens[i - 1].type != NUMBER &&tokens[i-1].type!=')')) )
      tokens[i].type=deref;
  }

  unsigned int ans=calculate(0,nr_token);
  *if_success = true;
  return ans;
}

int check_(int count,int q){
  int tmp=0;
  for(int i = count;i < q;i ++ ){
    if(tokens[i].type=='(')
      tmp+=1;
    else if(tokens[i].type==')')
      tmp-=1;
    if(tmp==0){
      tmp=i;
      break;
    }
  }
  return tmp;
}

unsigned int calculate(int p,int q){
  if(p >= q){
    printf("wrong");
    return 0;
  }
  int count=p,num_of_stack=0,sign[100]={0},tmp=0,i=0;
  unsigned int temp1=0,temp2=0,temp=0,stack[100]={0},val1=0,val2=0;
  bool flag=true;
  memset(sign,0,sizeof(sign));
  memset(stack,0,sizeof(stack));
  while(count < q){
    count++;
    switch(tokens[count-1].type){
      case 257:
          stack[num_of_stack++]=cpu.pc;
          break;
      case 253:
          tmp=0;
          for(i = count;i < q;i ++ ){
            if(tokens[i].type=='(')
              tmp+=1;
            else if(tokens[i].type==')')
              tmp-=1;
            if(tmp==0&&(tokens[i].type==NUMBER||tokens[i].type==')')){
              tmp=i;
              break;
            }
          }
          temp=(-1U)*calculate(count,tmp+1);
          tokens[tmp].type=NUMBER;
          sprintf(tokens[tmp].str,"%u",temp);
          count=tmp;
          break;
      case 255:
          tmp=check_(count,q);
          if(i == q)
            val2=calculate(count,q);
          else 
            val2=calculate(count,tmp+1);
          val1=stack[num_of_stack-1];
          stack[num_of_stack-1]=0;
          if(val1==val2)
            temp=1;
          else 
            temp=0;
          tokens[tmp].type=NUMBER;
          sprintf(tokens[tmp].str,"%u",temp);
          count=tmp;
          num_of_stack--;
          break;
      case 248:
          tmp=check_(count,q);
          if(i == q)
            val2=calculate(count,q);
          else 
            val2=calculate(count,tmp+1);
          val1=stack[num_of_stack-1];
          stack[num_of_stack-1]=0;
          if(val1==val2)
            temp=0;
          else 
            temp=1;
          tokens[tmp].type=NUMBER;
          sprintf(tokens[tmp].str,"%u",temp);
          count=tmp;
          num_of_stack--;
          break;
      case 249:
          tmp=check_(count,q);
          if(i == q)
            val2=calculate(count,q);
          else 
            val2=calculate(count,tmp+1);
          val1=stack[num_of_stack-1];
          stack[num_of_stack-1]=0;
          temp=val1&&val2;
          tokens[tmp].type=NUMBER;
          sprintf(tokens[tmp].str,"%u",temp);
          count=tmp;
          num_of_stack--;
          break;
      case 251:
          flag=true;
          stack[num_of_stack++]=isa_reg_str2val(tokens[count-1].str,&flag);
          break;
      case 250:
          tmp=check_(count,q);
          if(i == q) 
            temp=calculate(count,q);
          else 
            temp=calculate(count,tmp+1);
          tokens[tmp].type=NUMBER;
          sprintf(tokens[tmp].str,"%u",temp);
          count=tmp;
          break;
      case 256:
          break;
      case '+':
          stack[num_of_stack++]='+';
          sign[num_of_stack-1]='+';
          break;
      case '-':
          stack[num_of_stack++]='-';
          sign[num_of_stack-1]='-';
          break; 
      case '*':
          stack[num_of_stack++]='*';
          sign[num_of_stack-1]='*';
          break;
      case '/':
          stack[num_of_stack++]='/';
          sign[num_of_stack-1]='/';
          break;
      case 252:
          if(num_of_stack == 0)  {
              sscanf(tokens[count-1].str,"%x", &temp);
              stack[num_of_stack++]=temp;
              break;
            }
            num_of_stack-=1;
          if(sign[num_of_stack]=='*'||sign[num_of_stack]=='/'){
            temp1=stack[num_of_stack-1];
            sscanf(tokens[count-1].str,"%x", &temp2);
            if(sign[num_of_stack]=='*')
              temp1=temp1*temp2;
            else{
              if(temp2==0){
                printf("divided by 0\n");
                return 0;
              }
              temp1=temp1/temp2;
            }
            stack[num_of_stack-1]=temp1;
            sign[num_of_stack]=0;
            stack[num_of_stack]=0;
          }
          else{
            sscanf(tokens[count-1].str,"%u", &temp);
            stack[num_of_stack+1]=temp;
            num_of_stack+=2;
          }
          break;
      case 254:
          if(num_of_stack == 0)  {
              sscanf(tokens[count-1].str,"%u", &temp);
              stack[num_of_stack++]=temp;
              break;
            }
            num_of_stack-=1;
          if(sign[num_of_stack]=='*'||sign[num_of_stack]=='/'){
            temp1=stack[num_of_stack-1];
            sscanf(tokens[count-1].str,"%u", &temp2);
            if(sign[num_of_stack]=='*')
              temp1=temp1*temp2;
            else{
              if(temp2==0){
                printf("divided by 0\n");
                return 0;
              }
              temp1=temp1/temp2;
            }
            stack[num_of_stack-1]=temp1;
            sign[num_of_stack]=0;
            stack[num_of_stack]=0;
          }
          else{
            sscanf(tokens[count-1].str,"%u", &temp);
            stack[num_of_stack+1]=temp;
            num_of_stack+=2;
          }
          break;
      case '(':
          stack[num_of_stack++]='(';
          sign[num_of_stack-1]='(';
          break;
      case ')':
          temp=0;
          num_of_stack-=1;
          while(sign[num_of_stack]!='('){
            switch(sign[num_of_stack]){
              case '-':
                if(stack[num_of_stack-2]!='(' && sign[num_of_stack-2]==sign[num_of_stack]){
                  temp=stack[num_of_stack-1]+temp;
                  sign[num_of_stack]=0;
                  stack[num_of_stack]=0;
                  stack[num_of_stack-1]=temp;
                }     
                else if(stack[num_of_stack-2]!='('){
                  temp=stack[num_of_stack-1]-temp;
                  sign[num_of_stack]=0;
                  stack[num_of_stack]=0;
                  stack[num_of_stack-1]=temp;
                }
                else{
                  temp=stack[num_of_stack-1]-temp;
                  stack[num_of_stack-1]=temp;
                }
                sign[num_of_stack]=0;
                num_of_stack--;
                break;
              case '+':
                if(stack[num_of_stack-2]!='(' && sign[num_of_stack-2]==sign[num_of_stack]){
                  temp=stack[num_of_stack-1]+temp;
                  sign[num_of_stack]=0;
                  stack[num_of_stack]=0;
                  stack[num_of_stack-1]=temp;
                }
                else if(stack[num_of_stack-2]!='('){
                  temp=stack[num_of_stack-1]-temp;
                  sign[num_of_stack]=0;
                  stack[num_of_stack]=0;
                  stack[num_of_stack-1]=temp;
                }
                else{
                  temp=stack[num_of_stack-1]+temp;
                  stack[num_of_stack-1]=temp;
                }
                sign[num_of_stack]=0;
                num_of_stack--;
                break;
              default:
                temp=stack[num_of_stack];
                stack[num_of_stack]=0;
                num_of_stack--;
            }
          }
          sign[num_of_stack]=0;
          count-=1;
          tokens[count].type=254;
          sprintf(tokens[count].str,"%u",temp); 
          break;   
      default:
          assert(0);
    }
  } 
  /*
  *we have pushed all the vals into the stack.wo also dealt with the nums
  *of the call sign(translated by Bing)
  */
  temp=stack[--num_of_stack];
  while( num_of_stack > 0){
    switch(sign[num_of_stack]){
      case '-':
        if(num_of_stack > 2 && sign[num_of_stack-2]==sign[num_of_stack]){
          temp=stack[num_of_stack-1]+temp;
          sign[num_of_stack]=0;
          stack[num_of_stack]=0;
          stack[num_of_stack-1]=temp;
        }
        else if(num_of_stack >2){
          temp=stack[num_of_stack-1]-temp;
          sign[num_of_stack]=0;
          stack[num_of_stack]=0;
          stack[num_of_stack-1]=temp;
        }
        else{
          temp=stack[num_of_stack-1]-temp;
          stack[num_of_stack-1]=temp;
        }
        num_of_stack--;
        break;
      case '+':
        if(num_of_stack > 2 && sign[num_of_stack-2]==sign[num_of_stack]){
          temp=stack[num_of_stack-1]+temp;
          sign[num_of_stack]=0;
          stack[num_of_stack]=0;
          stack[num_of_stack-1]=temp;
        }
        else if(num_of_stack >2){
          temp=stack[num_of_stack-1]-temp;
          sign[num_of_stack]=0;
          stack[num_of_stack]=0;
          stack[num_of_stack-1]=temp;
        }
        else{
          temp=stack[num_of_stack-1]+temp;
          stack[num_of_stack-1]=temp;
        }
        num_of_stack--;
        break;
      default:
        temp=stack[num_of_stack];
        stack[num_of_stack]=0;
        num_of_stack--;
    }
  }
  return temp;
}

