#ifndef ECM_H
#define ECM_H

#include <limits.h>
#include "utils/material.h"
#include "utils/texture.h"
#include "utils/mesh.h"
#include "entity.h"
#include "utils/vector.h"
#include "utils/khash.h"
#include "utils/macros.h"

#define _GNU_SOURCE
#include <search.h>

typedef unsigned long ulong;

typedef struct ecm_t ecm_t;

typedef struct c_t c_t;

typedef void(*init_cb)(c_t *self);
typedef void(*destroy_cb)(c_t *self);
typedef int(*signal_cb)(c_t *self, void *data, void *output);
typedef void(*c_reg_cb)(void);

/* TODO: find appropriate place */
typedef int(*before_draw_cb)(c_t *self);


#define IDENT_NULL UINT_MAX
#define DEF_CASTER(ct, cn, nc_t) \
	static inline nc_t *cn(void *entity)\
{ return (nc_t*)ct_get(ecm_get(ref(ct)), entity); }

#define CONSTR_BEFORE_REG 102
#define CONSTR_REG 103
#define CONSTR_AFTER_REG 104
/* #define REG() \ */
	/* static void _register(void); \ */
	/* __attribute__((constructor (CONSTR_REG))) static void add_reg(void) \ */
	/* { ecm_add_reg(_register); } \ */
	/* static void _register(void) */
#define REG() \
	__attribute__((constructor (CONSTR_REG))) static void _register(void)

#define ecm_foreach_c(vvar, code) { khint_t __i;		\
	for (__i = kh_begin(g_ecm->cs); __i != kh_end(g_ecm->cs); ++__i) {		\
		if (!kh_exist(g_ecm->cs,__i)) continue;						\
		(vvar) = &kh_val(g_ecm->cs,__i);								\
		code;												\
	} }

#define ecm_foreach_ct(vvar, code) { khint_t __i;		\
	for (__i = kh_begin(g_ecm->cts); __i != kh_end(g_ecm->cts); ++__i) {		\
		if (!kh_exist(g_ecm->cts,__i)) continue;						\
		(vvar) = &kh_val(g_ecm->cts,__i);								\
		code;												\
	} }

#define CONTINUE			0x0000
#define STOP				0x0001
#define RENEW				0x0010

#define WORLD				0x0000
#define ENTITY				0x0100

/* #define RENDER_THREAD 0x02 */

typedef struct
{
	uint signal;
	signal_cb cb;
	int flags;
	uint target;
	int priority;
} listener_t;

typedef struct
{
	uint ct;
	int is_interaction;
} dep_t;

#define PAGE_SIZE 32

struct comp_page
{
	char *components;
	uint components_size;
};

KHASH_MAP_INIT_INT(c, c_t*)

typedef struct ct_t
{
	char name[32];

	init_cb init;
	destroy_cb destroy;

	uint id;
	uint size;

	khash_t(c) *cs;
	/* struct comp_page *pages; */
	/* uint pages_size; */

	dep_t *depends;
	uint depends_size;

	int is_interaction;

} ct_t;

typedef struct
{
	uint size;

	vector_t *listener_types;
	vector_t *listener_comps;

} signal_t;

KHASH_MAP_INIT_INT(sig, signal_t)
KHASH_MAP_INIT_INT(ct, ct_t)

typedef struct
{
	unsigned int next_free;
	unsigned int uid;
} entity_info_t;

typedef struct ecm_t
{
	entity_info_t *entities_info;
	uint entities_info_size;
	uint entity_uid_counter;

	khash_t(ct) *cts;
	khash_t(c) *cs;

	khash_t(sig) *signals;

	uint global;

	int regs_size;
	c_reg_cb *regs;

	int dirty;
	int safe;
} ecm_t; /* Entity Component System */

typedef struct c_t
{
	entity_t entity;
	uint comp_type;
	uint ghost;
} c_t;

#define c_entity(c) (((c_t*)c)->entity)

#define _type(a, b) __builtin_types_compatible_p(typeof(a), b)
#define _if(c, a, b) __builtin_choose_expr(c, a, b)

/* static inline c_t *ct_get_at(ct_t *self, uint page, uint i) */
/* { */
	/* return (c_t*)&(self->pages[page].components[i * self->size]); */
/* } */

signal_t *ecm_get_signal(uint signal);
void _signal_init(uint id, uint size);
#define signal_init(id, size) \
	(_signal_init(id, size))

void _ct_listener(ct_t *self, int flags, uint signal, signal_cb cb);
#define ct_listener(self, flags, signal, cb) \
	(_ct_listener(self, flags, signal, (signal_cb)cb))

listener_t *ct_get_listener(ct_t *self, uint signal);

/* void ct_register_callback(ct_t *self, uint callback, void *cb); */

c_t *ct_add(ct_t *self, entity_t entity);

extern ecm_t *g_ecm;
void ecm_init(void);
entity_t ecm_new_entity(void);
void ecm_add_reg(c_reg_cb reg);
void ecm_register_all(void);
void ecm_clean(int force);
void ecm_destroy_all(void);

void ecm_add_entity(entity_t *entity);
/* uint ecm_register_system(ecm_t *self, void *system); */

#define ct_new(name, size, init, destroy, depend_size, ...) \
	_ct_new(name, ref(name), size, (init_cb)init, (destroy_cb)destroy, \
			depend_size, ##__VA_ARGS__)
ct_t *_ct_new(const char *name, uint hash, uint size, init_cb init,
		destroy_cb destroy, int depend_size, ...);

void ct_add_dependency(ct_t *dep, uint target);
void ct_add_interaction(ct_t *dep, uint target);


static inline ct_t *ecm_get(uint comp_type)
{
	khiter_t k = kh_get(ct, g_ecm->cts, comp_type);
	if(k == kh_end(g_ecm->cts)) return NULL;
	return &kh_value(g_ecm->cts, k);
}
/* void ecm_generate_hashes(ecm_t *self); */

#define component_new_from_ref(comp_type) _component_new(comp_type)
#define component_new(comp_type) _component_new(ref(comp_type))
void *_component_new(uint comp_type);

static inline unsigned int entity_exists(entity_t self)
{
	if(self == entity_null) return 0;
	entity_info_t info = g_ecm->entities_info[entity_pos(self)];
	return info.uid == entity_uid(self);
}

static inline c_t *ct_get_nth(ct_t *self, int n)
{
	khiter_t k;
	for(k = kh_begin(self->cs); k != kh_end(self->cs); ++k)
	{
		if(!kh_exist(self->cs, k)) continue;
		if(n == 0)
		{
			return kh_value(self->cs, k);
		}
		else
		{
			n--;
		}
	}
	return NULL;
}

static inline c_t *ct_get(ct_t *self, entity_t *entity)
{
	if(!entity_exists(*entity)) return NULL;
	khiter_t k = kh_get(c, self->cs, entity_uid(*entity));
	if(k == kh_end(self->cs)) return NULL;
	return kh_value(self->cs, k);
}


/* TODO maybe this shouldn't be here */
static inline vec2_t entity_to_vec2(entity_t self)
{
	int pos = entity_pos(self);

	union {
		unsigned int i;
		struct{
			unsigned char r, g, b, a;
		};
	} convert = {.i = pos};

	return vec2((float)convert.r / 255, (float)convert.g / 255);
}

#define SYS ((entity_t){0x0000000100000001})

/* builtin signals */

#endif /* !ECM_H */