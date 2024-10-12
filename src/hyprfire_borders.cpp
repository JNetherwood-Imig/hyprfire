#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <wayfire/core.hpp>
#include <wayfire/geometry.hpp>
#include <wayfire/opengl.hpp>
#include <wayfire/per-output-plugin.hpp>
#include <wayfire/view-transform.hpp>
#include <wayfire/view.hpp>
#include <wayfire/render-manager.hpp>
#include <wayfire/signal-definitions.hpp>

class rounded_corner_transformer_t
{
	rounded_corner_transformer_t(wayfire_view view)
	{
		this->view = view;
		auto transform_manager = view->get_transformed_node();
		auto transformer = ensure_transformer(view);
	}

	wf::post_hook_t render_hook = [=] (const wf::framebuffer_t& src, const wf::framebuffer_t& dest)
	{
		OpenGL::render_begin(dest);
		program.use(wf::TEXTURE_TYPE_RGBA);
		program.set_active_texture(src.tex);

		upload_data();

		GL_CALL(glEnable(GL_BLEND));
		GL_CALL(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));

		GL_CALL(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
		
		GL_CALL(glDisable(GL_BLEND));

		program.deactivate();
		OpenGL::render_end();
	};
	private:
	static constexpr const char* const transformer_name = "rounded-corners";
	wayfire_view view;
	OpenGL::program_t program;
	std::vector<GLfloat> vertex_data;

	std::shared_ptr<wf::scene::view_2d_transformer_t> ensure_transformer(wayfire_view view)
	{
		auto transform_manager = view->get_transformed_node();
		if (!transform_manager->get_transformer<wf::scene::node_t>(transformer_name))
			transform_manager->add_transformer(std::make_shared<wf::scene::view_2d_transformer_t>(view), wf::TRANSFORMER_2D - 1, transformer_name);
		return transform_manager->get_transformer<wf::scene::view_2d_transformer_t>(transformer_name);
	}

	void upload_data()
	{
		wf::geometry_t geometry = view->get_bounding_box();
		float x = geometry.x, y = geometry.y, w = geometry.width, h = geometry.height;
		vertex_data = {
			x, y + h,
			x + w, y + h,
			x + w, y,
			x, y
		};
	}
};

class hyprfire_borders_t : public wf::per_output_plugin_instance_t
{
	public:
	void init() override
	{ output->connect(&on_view_mapped); }

	void fini() override
	{ output->disconnect(&on_view_mapped); }

	private:
	wf::signal::connection_t<wf::view_mapped_signal> on_view_mapped = [=] (wf::view_mapped_signal* ev)
	{
	};
};

DECLARE_WAYFIRE_PLUGIN(wf::per_output_plugin_t<hyprfire_borders_t>);
