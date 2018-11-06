#include "mdcc.h"


static void expect(int expected, int actual) {
    if (expected == actual)
        return;
    fprintf(stderr,  "%d: %d expected, but got %d\n", __LINE__, expected, actual);
    exit(1);
}


static void test_vec() {
    Vector *v = new_vec();
    expect(0, v->len);
    vec_push(v, (void *)10);
    expect(1, v->len);
    expect(10, (int)vec_pop(v));
}

static void test_map() {
    Map *m = new_map();
    map_set(m, "a", (void *)1);
    map_set(m, "b", (void *)2);
    expect((int)map_get(m, "b"), 2);
    map_set(m, "a", (void *)3);
    expect((int)map_get(m, "a"), 3);
}

void test() {
    test_vec();
    test_map();
}
