#include "struct.h"
#include <assert.h>
#include <stdlib.h>

struct Account {
  unsigned int balance;
};

struct Account* create_account(unsigned int balance) {
  struct Account* account = malloc(sizeof(struct Account));
  account->balance = balance;
  return account;
}

void destroy_account(struct Account* account) {
  assert(account != NULL);
  free(account);
}

int widthdraw(struct Account* account, unsigned int amount) {
  assert(account != NULL);
  if (account->balance < amount) {
    return 1;
  }

  account->balance -= amount;
  return 0;
}

void deposit(struct Account* account, unsigned int amount) {
  assert(account != NULL);
  account->balance += amount;
}

unsigned int get_balance(struct Account* account) {
  return account->balance;
}
