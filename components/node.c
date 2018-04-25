#include "node.h"
#include "name.h"
#include "spacial.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static void c_node_init(c_node_t *self)
{
	self->children = NULL;
	self->children_size = 0;
	self->model = mat4();
	self->cached = 0;
	self->parent = entity_null;
	self->inherit_scale = 1;
}

c_node_t *c_node_new()
{
	c_node_t *self = component_new("node");
	return self;
}

static int c_node_changed(c_node_t *self)
{
	ulong i;
	self->cached = 0;
	for(i = 0; i < self->children_size; i++)
	{
		c_node_changed(c_node(&self->children[i]));
	}
	return CONTINUE;
}

entity_t c_node_get_by_name(c_node_t *self, uint hash)
{
	ulong i;
	for(i = 0; i < self->children_size; i++)
	{
		entity_t child = self->children[i];
		c_name_t *child_name = c_name(&child);
		c_node_t *child_node;

		if(child_name && child_name->hash == hash)
		{
			return child;
		}

		child_node = c_node(&child);
		if(child_node)
		{
			entity_t response = c_node_get_by_name(child_node, hash);
			if(response != entity_null)
			{
				return response;
			}
		}
	}
	return entity_null;
}

void c_node_unparent(c_node_t *self, int inherit_transform)
{
	if(self->parent)
	{
		c_node_t *parent = c_node(&self->parent);
		if(inherit_transform)
		{
			int prev_inheritance = self->inherit_scale;
			self->inherit_scale = 0;
			self->cached = 0;
			c_node_update_model(self);

			c_spacial_set_model(c_spacial(self), self->model);
			self->inherit_scale = prev_inheritance;
		}
		c_node_remove(parent, c_entity(self));
	}
}

void c_node_remove(c_node_t *self, entity_t child)
{
	int i;
	for(i = 0; i < self->children_size; i++)
	{
		if(self->children[i] == child)
		{
			c_node_t *cn = c_node(&child);
			cn->cached = 0;
			cn->parent = entity_null;
			if(--self->children_size)
			{
				self->children[i] = self->children[self->children_size];
			}
		}

	}
}

void c_node_add(c_node_t *self, int num, ...)
{
	va_list children;
	va_start(children, num);
	while(num--)
	{
		entity_t child = va_arg(children, entity_t);
		c_node_t *child_node = c_node(&child);
		if(!child_node)
		{
			entity_add_component(child, c_node_new());
			child_node = c_node(&child);
		}
		else if(child_node->parent)
		{
			if(child_node->parent == c_entity(self))
			{
				continue;
			}
			c_node_remove(c_node(&child_node->parent), child);
		}

		int i = self->children_size++;
		self->children = realloc(self->children,
				(sizeof *self->children) * self->children_size);
		self->children[i] = child;
		child_node->parent = c_entity(self);

		c_node_changed(child_node);
	}
	va_end(children);
}

REG()
{
	/* TODO destroyer */
	ct_t *ct = ct_new("node", sizeof(c_node_t),
			c_node_init, NULL, 1, ref("spacial"));

	ct_listener(ct, ENTITY, sig("spacial_changed"), c_node_changed);
}

void c_node_update_model(c_node_t *self)
{
	if(self->cached) return;
	self->cached = 1;

	entity_t parent = self->parent;
	c_spacial_t *sc = c_spacial(self);

	if(self->parent != entity_null)
	{
		c_node_t *parent_node = c_node(&parent);
		c_node_update_model(parent_node);

		self->rot = mat4_mul(parent_node->rot, sc->rot_matrix);
#ifdef MESH4
		self->angle4 = parent_node->angle4 + sc->angle4;
#endif

		if(self->inherit_scale)
		{
			self->model = mat4_mul(parent_node->model, sc->model_matrix);
		}
		else
		{
			vec3_t pos = mat4_mul_vec4(parent_node->model, vec4(_vec3(sc->pos), 1.0f)).xyz;

			mat4_t model = mat4_translate(pos);
			model = mat4_mul(model, parent_node->rot);

			self->model = mat4_mul(model, sc->rot_matrix);
		}
	}
	else
	{
		self->model = sc->model_matrix;
		self->rot = sc->rot_matrix;
#ifdef MESH4
		self->angle4 = sc->angle4;
#endif
	}
}

vec3_t c_node_global_to_local(c_node_t *self, vec3_t vec)
{
	mat4_t inv;
	c_node_update_model(self);
	inv = mat4_invert(self->model);
	return mat4_mul_vec4(inv, vec4(vec.x, vec.y, vec.z, 1.0)).xyz;
}

vec3_t c_node_local_to_global(c_node_t *self, vec3_t vec)
{
	c_node_update_model(self);
	return mat4_mul_vec4(self->model, vec4(vec.x, vec.y, vec.z, 1.0)).xyz;
}

vec3_t c_node_dir_to_local(c_node_t *self, vec3_t vec)
{
	mat4_t inv;
	c_node_update_model(self);
	inv = mat4_invert(self->model);
	return mat4_mul_vec4(inv, vec4(vec.x, vec.y, vec.z, 0.0)).xyz;
}

vec3_t c_node_dir_to_global(c_node_t *self, vec3_t vec)
{
	c_node_update_model(self);
	return mat4_mul_vec4(self->model, vec4(vec.x, vec.y, vec.z, 0.0)).xyz;
}
