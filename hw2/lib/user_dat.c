#include "user_dat.h"
HashTable *user_passwords;
void user_dat_init() {
    user_passwords = table_create(NULL);
    table_emplace(user_passwords, "ray1422", "qewrty", sizeof("qewrty"));
    table_emplace(user_passwords, "jw910731", "asdf", sizeof("asdf"));
}