#include "crossfade_2.hpp"

#include <wayfire/plugins/wobbly/wobbly-signal.hpp>
#include <wayfire/window-manager.hpp>
#include <wayfire/plugins/common/util.hpp>

namespace wf::hyprfire
{
    TransformNode::TransformNode(const wayfire_toplevel_view view) : view_2d_transformer_t(view)
    {
        displayed_geometry = view->get_geometry();
        overlay_alpha = 0;
        this->view = view;

        const auto root_node = view->get_surface_root_node();
        const geometry_t bbox = root_node->get_bounding_box();

        original_buffer.geometry = view->get_geometry();
        original_buffer.scale    = view->get_output()->handle->scale;

        OpenGL::render_begin();
        const auto w = static_cast<int>(original_buffer.scale * static_cast<float>(original_buffer.geometry.width));
        const auto h = static_cast<int>(original_buffer.scale * static_cast<float>(original_buffer.geometry.height));
        original_buffer.allocate(w, h);
        OpenGL::render_end();

        std::vector<scene::render_instance_uptr> instances;
        root_node->gen_render_instances(instances, [] (auto) {}, view->get_output());

        scene::render_pass_params_t params;
        params.background_color = {0, 0, 0, 0};
        params.damage    = bbox;
        params.target    = original_buffer;
        params.instances = &instances;
        run_render_pass(params, scene::RPASS_CLEAR_BACKGROUND);
    }

    TransformNode::~TransformNode()
    {
        OpenGL::render_begin();
        original_buffer.release();
        OpenGL::render_end();
    }

    std::string TransformNode::stringify() const { return "crossfade"; }

    inline void TransformNode::gen_render_instances(std::vector<scene::render_instance_uptr>& instances, scene::damage_callback push_damage, output_t *shown_on)
    {
        instances.push_back(std::make_unique<TransformRenderInstance>(this, push_damage));

        view_2d_transformer_t::gen_render_instances(instances, push_damage, shown_on);
    }

    TransformAnimation::TransformAnimation(const wayfire_toplevel_view view, const option_sptr_t<animation_description_t>& duration)
    {
        this->view   = view;
        this->output = view->get_output();
        this->animation = geometry_animation_t{duration};

        output->render->add_effect(&pre_hook, OUTPUT_EFFECT_PRE);
        output->connect(&on_disappear);
    }

    TransformAnimation::~TransformAnimation()
    {
        view->get_transformed_node()->rem_transformer<TransformNode>();
        output->render->rem_effect(&pre_hook);
    }

    void TransformAnimation::adjust_target_geometry(const geometry_t geometry, const int32_t target_edges, const txn::transaction_uptr& tx)
    {
        // Apply the desired attributes to the view
        const auto& set_state = [&]
        {
            if (target_edges >= 0)
            {
                get_core().default_wm->update_last_windowed_geometry(view);
                view->toplevel()->pending().fullscreen  = false;
                view->toplevel()->pending().tiled_edges = target_edges;
            }

            view->toplevel()->pending().geometry = geometry;
            tx->add_object(view->toplevel());
        };

        // Crossfade animation
        original = view->get_geometry();
        animation.set_start(original);
        animation.set_end(geometry);
        activate_wobbly(view);
        animation.start();

        // Add crossfade transformer
        ensure_view_transformer<TransformNode>(view, TRANSFORMER_2D, view);

        // Start the transition
        set_state();
    }

    void TransformAnimation::adjust_target_geometry(const geometry_t geometry, const int32_t target_edges)
    {
        auto tx = txn::transaction_t::create();
        adjust_target_geometry(geometry, target_edges, tx);
        get_core().tx_manager->schedule_transaction(std::move(tx));
    }

    TransformRenderInstance::TransformRenderInstance(TransformNode *self,const scene::damage_callback& push_damage)
    {
        this->self = std::dynamic_pointer_cast<TransformNode>(self->shared_from_this());
        scene::damage_callback push_damage_child = [=] (const region_t&) { push_damage(self->get_bounding_box()); };

        on_damage = [=] (auto) { push_damage(self->get_bounding_box()); };
        self->connect(&on_damage);
    }

    void TransformRenderInstance::schedule_instructions(std::vector<scene::render_instruction_t>& instructions, const render_target_t& target, region_t& damage)
    {
        instructions.push_back(scene::render_instruction_t{
            .instance = this,
            .target   = target,
            .damage   = damage & self->get_bounding_box(),
        });
    }

    void TransformRenderInstance::render(const render_target_t& target, const region_t& region)
    {
        double render_alpha;
        constexpr double N = 2;
        if (self->overlay_alpha < 0.5)
            render_alpha = std::pow(self->overlay_alpha * 2, 1.0 / N) / 2.0;
        else
            render_alpha = std::pow((self->overlay_alpha - 0.5) * 2, N) / 2.0 + 0.5;

        OpenGL::render_begin(target);
        for (auto& box : region)
        {
            target.logic_scissor(wlr_box_from_pixman_box(box));
            OpenGL::render_texture({self->original_buffer.tex}, target, self->displayed_geometry, glm::vec4{1.0f, 1.0f, 1.0f, 1.0 - render_alpha});
        }

        OpenGL::render_end();
    }
}