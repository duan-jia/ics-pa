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

#include "sdb.h"
#include "watchpoint.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

WP* new_wp() { 
	assert(free_ != NULL);
	WP *temp = free_;
	free_ = free_->next;
	temp->next = head;
	head = temp;
	temp = NULL;
	return head;
}

void free_wp(WP *wp) {
	assert(head != NULL);
	if(head == wp) {
		head = wp->next;
	}
	else {
		WP *temp = head;
		while(temp->next != NULL && temp->next != wp) {
			temp = temp->next;
		}
		//assert(temp->next != NULL);
		temp->next = wp->next;
	}
	wp->next = free_;
	free_ = wp;
}
void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
bool check_all_wps() {
	WP *temp = head;
	while(temp) {
		bool is_success = false;
		word_t val = expr(temp->exp, &is_success);
		assert(is_success);
		if(val != temp->exp_val) {
			Log("Program hits watchpoint %d:%s\n", temp->NO, temp->exp);
			return false;
		}
		temp = temp->next;
	}
	return true;
}

void print_all_wps() {
	WP *temp = head;
	while(temp) {
		Log("watchpoint %d:%s\n", temp->NO, temp->exp);
		temp = temp->next;
	}
}

void delete_wp(int n) {
	WP *temp = head;
	while(temp) {
		if(temp->NO == n) {
			free_wp(temp);
			printf("the watchpoint %d is successfully delete\n", n);
			break;
		}
		else {
			temp = temp->next;
		}
	}
	if(temp == NULL) {
		printf("Error:cannot find the watchpoint %d \n", n);
	}
}
