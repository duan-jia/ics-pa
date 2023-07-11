#ifndef WATCHPOINT_H_
#define WATCHPOINT_H_

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
	char exp[32];
	word_t exp_val;
} WP;

WP* new_wp();
void delete_wp(int n);
bool check_all_wps();
void print_all_wps();
#endif
