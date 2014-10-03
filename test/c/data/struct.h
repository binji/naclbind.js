struct Account;

struct Account* create_account(unsigned int balance);
void destroy_account(struct Account*);
int widthdraw(struct Account*, unsigned int amount);
void deposit(struct Account*, unsigned int amount);
unsigned int get_balance(struct Account*);
