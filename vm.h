#ifndef BRD_VM_H
#define BRD_VM_H

/* 
 * If I were smarter I'd make the bucket grow
 * But for the mean time that feels like a premature optimization
 */
#define BUCKET_SIZE 32

enum brd_value_type {
        BRD_VAL_NUM,
        BRD_VAL_STRING,
        BRD_VAL_BOOL,
        BRD_VAL_UNIT,
};

struct brd_value {
        enum brd_value_type vtype;
        union {
                long double num;
                char *string;
                int boolean;
        } as;
};

struct brd_value_map_list {
        char *key;
        struct brd_value val;
        struct brd_value_map_list *next;
};

struct brd_value_map {
        struct brd_value_map_list bucket[BUCKET_SIZE];
};

void brd_value_map_init(struct brd_value_map *map);
void brd_value_map_destroy(struct brd_value_map *map);
void brd_value_map_set(struct brd_value_map *map, char *key, struct brd_value *val);
struct brd_value *brd_value_map_get(struct brd_value_map *map, char *key);

#endif
