#include "user_dat.h"
#include <stdio.h>
#include <assert.h>

int main() {
    assert(user_check("asdf", "asdf"));
    assert(user_check("asdf", "qwerty") == 0);
}