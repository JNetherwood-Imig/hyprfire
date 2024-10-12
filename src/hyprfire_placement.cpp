#include <wayfire/toplevel-view.hpp>
#include <wayfire/workarea.hpp>
#include <wayfire/per-output-plugin.hpp>

namespace wf
{
	class hyprfire_placement_t : public per_output_plugin_instance_t
	{
		signal::connection_t<view_mapped_signal> on_view_mapped = [=] (view_mapped_signal* ev)
		{
			auto toplevel = toplevel_cast(ev->view);
			ev->is_positioned = true;
			place(toplevel, output->workarea->get_workarea());
		};

		public:
		void init() override
		{ output->connect(&on_view_mapped); }

		void fini() override
		{ output->disconnect(&on_view_mapped); }

		private:
		void place(wayfire_toplevel_view& view, geometry_t workarea)
		{
			geometry_t window = view->get_pending_geometry();
			pointf_t cursor_pos = get_core().get_cursor_position();
			window.x = cursor_pos.x - (window.width / 2.0);
			window.y = cursor_pos.y - (window.height / 2.0);
			view->move(window.x, window.y);
		}
	};

	DECLARE_WAYFIRE_PLUGIN(per_output_plugin_t<hyprfire_placement_t>);
}
