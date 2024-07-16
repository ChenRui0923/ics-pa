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

#define NR_WP 32

word_t expr(char *e, bool *success);

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char exp[128];
  uint32_t last;
  /* TODO: Add more members if necessary */
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL, *curr = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    strcpy(wp_pool[i].exp, "\0");
    wp_pool[i].last = 0;
  }

  head = NULL;
  free_ = wp_pool;
}
WP* return_(){
  return head;
}
WP* search_(WP *wp){
  WP* temp=head;
  while(temp){
    if(temp->next==wp) break;
    temp=temp->next;
  }
  return temp;
}
WP* new_wp(char *expr_) {
  if (free_ == NULL) {
    printf("No free watchpoint available!\n");
    return NULL;
  }
  
  // 从free_链表中取出一个新的watchpoint
  WP* new_wp = free_;
  free_ = free_->next;

  // 初始化新的watchpoint
  new_wp->next = NULL;
  strcpy(new_wp->exp, expr_);
  bool success;
  new_wp->last = expr(new_wp->exp, &success);

  if (!success) {
    printf("The expression is invalid!\n");
    new_wp->next = free_;
    free_ = new_wp;
    return NULL;
  }

  // 将新的watchpoint加入到head链表中
  if (head == NULL) {
    head = new_wp;
  } else {
    curr->next = new_wp;
  }
  curr = new_wp;

  return new_wp;
}
void free_wp(int des){
  WP *wp=head;
  while(wp!=NULL){
    if(wp->NO==des) break;
    else wp=wp->next;
  }
  if(wp==head){
    head=head->next;
    wp->next=free_;
    free_=wp;
  }
  else if(wp==curr){
    wp->next=free_;
    free_=wp;
    curr=search_(wp);
    curr->next=NULL;
  }
  else{
    search_(wp)->next=wp->next;
    wp->next=free_;
    free_=wp;
  }
};
/* TODO: Implement the functionality of watchpoint */
void check_wp(){
  bool success;
  WP *itr = head;
  while(itr != NULL){
    uint32_t res = expr(itr->exp,&success);
    if(!success){
      printf("The expression of watch point %d is invalid!\n",itr->NO);
    }
    else if(res != itr->last){
      printf("Num:%-6d\t Expr:%-20s\t  New Val:%-14u\t  Old Val:%-14u\n",
        itr->NO,
        itr->exp,
        res,
        itr->last);
      itr->last = res;
      nemu_state.state = NEMU_STOP;
    }
    itr=itr->next;
  }
}
void print_wp(){
  WP *itr = head;
  if(head==NULL) {
    printf("there is no watchpoint in the pool");
    return;
  }
  printf("%-15s%-7s%s\n","NO","Exp","Val");
  while(itr != NULL){
    printf("%02d\t%10s\t%-10u\n",itr->NO,itr->exp,itr->last);
    itr = itr->next;
  }
}
