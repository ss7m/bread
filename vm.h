#ifndef FLT_VM_H
#define FLT_VM_H

/* 
 * If I were smarter I'd make the bucket grow
 * But for the mean time that feels like a premature optimization
 */
#define BUCKET_SIZE 32

enum flt_value_type {
        FLT_VAL_NUM,
        FLT_VAL_STRING,
        FLT_VAL_BOOL,
        FLT_VAL_UNIT,
};

struct flt_value {
        enum flt_value_type vtype;
        union {
                long double num;
                char *string;
                int boolean;
        } as;
};

struct flt_value_map_list {
        char *key;
        struct flt_value val;
        struct flt_value_map_list *next;
};

struct flt_value_map {
        struct flt_value_map_list bucket[BUCKET_SIZE];
};

void flt_value_map_init(struct flt_value_map *map);
void flt_value_map_destroy(struct flt_value_map *map);
void flt_value_map_set(struct flt_value_map *map, char *key, struct flt_value *val);
struct flt_value *flt_value_map_get(struct flt_value_map *map, char *key);

#endif
