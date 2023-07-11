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
#include <./memory/paddr.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_UEQ, TK_AND, TK_DEREF, TK_DEC, TK_HEX, TK_REG

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
	{"!=", TK_UEQ},       // unequal
	{"&&", TK_AND},
	{"\\-", '-'},
	{"\\*", '*'},
	{"/", '/'},
	{"\\(", '('},
	{"\\)", ')'},
	{"0x[A-Fa-f0-9]+", TK_HEX},
	{"[0-9]+", TK_DEC},
	{"\\$[a-zA-Z0-9]+", TK_REG}
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
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;
				
				assert(substr_len < 32);
				tokens[nr_token].type = rules[i].token_type;
				memcpy(tokens[nr_token].str, substr_start, sizeof(char) * substr_len);
				tokens[nr_token].str[substr_len] = '\0';

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
					case TK_NOTYPE:
						nr_token--;
						break;
        }
				nr_token++;
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

bool check_parenthesis(int p, int q) {
	assert(p < q);
	int par_level = 0;
	for(int i = p; i <= q; i++) {
		if(tokens[i].type == '(') {
			par_level++;
		}
		else if(tokens[i].type == ')') {
			par_level--;
			assert(par_level >= 0);
			if(par_level == 0 && i != q) {
				return false;
			}
		}
	}
	
	if(tokens[p].type != '(' || tokens[q].type != ')') return false;
	assert(par_level == 0);
	return true;
}

int find_main_op(int p, int q) {
	int main_op = -1, par_level = 0, priority = 4;
	for(int i = p; i <= q; i++) {
		if(tokens[i].type == TK_DEC) continue;
		else if(tokens[i].type == '(') {
			par_level++;
			continue;
		}
		else if (tokens[i].type == ')') {
			par_level--;
			continue;
		}
		if(par_level) continue;
		
		if((tokens[i].type == TK_DEREF) && priority >= 4) {
			main_op = i;
			priority = 4;
		}
		else if((tokens[i].type == '*' || tokens[i].type == '/') && priority >= 3) {
			main_op = i;
			priority = 3;
		}
		else if((tokens[i].type == '+' || tokens[i].type == '-') && priority >= 2) {
			main_op = i;
			priority = 2;
		}
		else if((tokens[i].type == TK_AND) && priority >= 1) {
			main_op = i;
			priority = 1;
		}
		else if((tokens[i].type == TK_EQ || tokens[i].type == TK_UEQ) && priority >= 0) {
			main_op = i;
			priority = 0;
		}
	}
	return main_op;
}

word_t eval(int p, int q){
	if(p > q) {
		return -1;
	}
	else if (p == q) {
		if(tokens[p].type == TK_DEC) {
			return strtoul(tokens[p].str, NULL, 10);
		}
		else if(tokens[p].type == TK_HEX) {
			return strtoul(tokens[p].str, NULL, 16);
		}
		else if(tokens[p].type == TK_REG) {
			bool success = false;
			word_t val = isa_reg_str2val(tokens[p].str, &success);
			if(success == false) {
				printf("Invalid Reg Expression\n");
				return 0;
			}
			return val;
		}
		assert(0);
	}
	else if(check_parenthesis(p, q)) {
		return eval(p + 1, q - 1);
	}
	else {
		int op = find_main_op(p, q);
		word_t val1 = eval(p, op - 1);
		word_t val2 = eval(op + 1, q);
		switch(tokens[op].type){
			case '+' :
				return val1 + val2;
			case '-' :
				return val1 - val2;
			case '*' :
				return val1 * val2;
			case '/' :
				if(val2 == 0){
					printf("the dividend num cannot be zero, asshole");
					return -1;
				}
				return val1 / val2;
			case TK_DEREF :
				return paddr_read(val2, 1);//bugs: the val1 is useless
			case TK_AND :
				return val1 && val2;
			case TK_EQ :
				return val1 == val2;
			case TK_UEQ :
				return val1 != val2;
			default : assert(0);
		}
	}
	return -1;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  *success = true;
  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
	for(int i = 0; i < nr_token; i++) {
		if(tokens[i].type == '*' && (i == 0 && 
					(tokens[i - 1].type != TK_DEC &&
					 tokens[i - 1].type != TK_HEX &&
					 tokens[i - 1].type != ')'))) {
					tokens[i].type = TK_DEREF;
		}
	}
  return eval(0, nr_token - 1);
}
