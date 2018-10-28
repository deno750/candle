#include "light.h"
#include "node.h"
#include "spacial.h"
#include "model.h"
#include <utils/drawable.h>
#include <candle.h>
#include <systems/editmode.h>
#include <systems/window.h>
#include <stdlib.h>
#include <systems/render_device.h>

static mesh_t *g_light;
extern vs_t *g_quad_vs;
extern mesh_t *g_quad_mesh;
/* entity_t g_light = entity_null; */
static int g_lights_num;

static int c_light_position_changed(c_light_t *self);

c_light_t *c_light_new(float radius, vec4_t color, uint32_t shadow_size)
{
	c_light_t *self = component_new("light");

	self->color = color;
	self->shadow_size = shadow_size;
	self->radius = radius;

	return self;
}

void c_light_init(c_light_t *self)
{
	self->color = vec4(1.0f);
	self->shadow_size = 512;
	self->radius = 5.0f;
	self->visible = 1;

	self->ambient_group = ref("ambient");
	self->light_group = ref("light");

	if(!g_light)
	{

		g_light = mesh_new();
		mesh_lock(g_light);
		mesh_ico(g_light, -0.5f);
		mesh_select(g_light, SEL_EDITING, MESH_FACE, -1);
		mesh_subdivide(g_light, 1);
		mesh_spherize(g_light, 1.0f);
		mesh_unlock(g_light);

		/* g_light = entity_new(c_node_new(), c_model_new(mesh, NULL, 0, 0)); */
		/* c_node(&g_light)->ghost = 1; */

	}
	self->id = g_lights_num++;

	drawable_init(&self->draw, 0, NULL);
	drawable_set_vs(&self->draw, g_model_vs);
	drawable_set_mesh(&self->draw, g_light);
	drawable_set_mat(&self->draw, self->id);
	drawable_set_entity(&self->draw, c_entity(self));

	c_light_position_changed(self);

	world_changed();
}

static void c_light_create_renderer(c_light_t *self)
{
	renderer_t *renderer = renderer_new(1.0f);

	texture_t *output =	texture_cubemap(self->shadow_size, self->shadow_size, 1);

	renderer_add_pass(renderer, "depth", "depth", ref("visible"),
			CULL_DISABLE, output, output, 0,
			(bind_t[]){
				{CLEAR_DEPTH, .number = 1.0f},
				{CLEAR_COLOR, .vec4 = vec4(0.0f)},
				{NONE}
			}
	);

	renderer_resize(renderer, self->shadow_size, self->shadow_size);

	renderer_set_output(renderer, output);

	self->renderer = renderer;
}

void c_light_visible(c_light_t *self, uint32_t visible)
{
	self->visible = visible;
	drawable_set_mesh(&self->draw, visible ? g_light : NULL);
}

static int c_light_position_changed(c_light_t *self)
{
	if(self->radius == -1)
	{
		drawable_set_group(&self->draw, self->ambient_group);
		drawable_set_vs(&self->draw, g_quad_vs);
		if(self->visible)
		{
			drawable_set_mesh(&self->draw, g_quad_mesh);
		}
	}
	else
	{
		drawable_set_group(&self->draw, self->light_group);
		drawable_set_vs(&self->draw, g_model_vs);
		if(self->visible)
		{
			drawable_set_mesh(&self->draw, g_light);
		}

		c_node_t *node = c_node(self);
		c_node_update_model(node);
		vec3_t pos = c_node_local_to_global(node, vec3(0, 0, 0));
		mat4_t model = mat4_translate(pos);
		model = mat4_scale_aniso(model, vec3(self->radius * 1.15f));

		drawable_set_transform(&self->draw, model);

		if(self->renderer)
		{
			renderer_set_model(self->renderer, 0, &node->model);
		}
	}
	if(self->radius > 0 && !self->renderer)
	{
		c_light_create_renderer(self);
	}

	return CONTINUE;
}

int c_light_menu(c_light_t *self, void *ctx)
{
	nk_layout_row_dynamic(ctx, 0, 1);

	int ambient = self->radius == -1.0f;
	nk_checkbox_label(ctx, "ambient", &ambient);

	if(!ambient)
	{
		float rad = self->radius;
		if(self->radius < 0.0f) self->radius = 0.01;
		nk_property_float(ctx, "radius:", 0.01, &rad, 1000, 0.1, 0.05);
		if(rad != self->radius)
		{
			self->radius = rad;
			c_light_position_changed(self);
			world_changed();
		}
	}
	else
	{
		self->radius = -1;
	}

	nk_layout_row_dynamic(ctx, 180, 1);

	struct nk_colorf *old = (struct nk_colorf *)&self->color;
	union { struct nk_colorf nk; vec4_t v; } new =
		{ .nk = nk_color_picker(ctx, *old, NK_RGBA)};

	if(memcmp(&new, &self->color, sizeof(vec4_t)))
	{
		self->color = new.v;
		world_changed();
	}

	return CONTINUE;
}

int c_light_draw(c_light_t *self)
{
	if(self->visible && self->renderer && self->radius > 0)
	{
		renderer_draw(self->renderer);
	}
	return CONTINUE;
}

void c_light_destroy(c_light_t *self)
{
}

REG()
{
	ct_t *ct = ct_new("light", sizeof(c_light_t), c_light_init,
			c_light_destroy, 1, ref("node"));

	ct_listener(ct, WORLD, sig("component_menu"), c_light_menu);
	ct_listener(ct, WORLD | 30, sig("world_draw"), c_light_draw);
	ct_listener(ct, ENTITY, sig("node_changed"), c_light_position_changed);
}
