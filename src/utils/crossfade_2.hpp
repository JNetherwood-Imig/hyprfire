#pragma once

#include <wayfire/signal-definitions.hpp>
#include <wayfire/toplevel-view.hpp>
#include <wayfire/plugins/common/geometry-animation.hpp>
#include <wayfire/render-manager.hpp>
#include <wayfire/txn/transaction-manager.hpp>

namespace wf::hyprfire
{
class TransformNode : public scene::view_2d_transformer_t
{
  public:
    wayfire_view view;
    render_target_t original_buffer;
    geometry_t displayed_geometry{};
    double overlay_alpha;

    explicit TransformNode(wayfire_toplevel_view view);
    ~TransformNode() override;

    std::string stringify() const override;

    void gen_render_instances(std::vector<scene::render_instance_uptr>& instances, scene::damage_callback push_damage, output_t *shown_on) override;
};

class TransformRenderInstance : public scene::render_instance_t
{
    std::shared_ptr<TransformNode> self;
    signal::connection_t<scene::node_damage_signal> on_damage;

  public:
    TransformRenderInstance(TransformNode *self,const scene::damage_callback& push_damage);

    void schedule_instructions(std::vector<scene::render_instruction_t>& instructions, const render_target_t& target, region_t& damage) override;

    void render(const render_target_t& target, const region_t& region) override;
};

class TransformAnimation : public custom_data_t
{
  public:

    TransformAnimation(wayfire_toplevel_view view, const option_sptr_t<animation_description_t>& duration);
    ~TransformAnimation() override;

    TransformAnimation(const TransformAnimation &) = delete;
    TransformAnimation(TransformAnimation &&) = delete;
    TransformAnimation& operator =(const TransformAnimation&) = delete;
    TransformAnimation& operator =(TransformAnimation&&) = delete;

    void adjust_target_geometry(geometry_t geometry, int32_t target_edges, const txn::transaction_uptr& tx);
    void adjust_target_geometry(geometry_t geometry, int32_t target_edges);

  protected:
    geometry_t original{};
    wayfire_toplevel_view view;
    output_t *output;
    geometry_animation_t animation;

    void destroy() const { view->erase_data<TransformAnimation>(); }

    effect_hook_t pre_hook = [=]
    {
        if (!animation.running())
            return destroy();

        if (view->get_geometry() != original)
        {
            original = view->get_geometry();
            animation.set_end(original);
        }

        const auto transform_node = view->get_transformed_node()->get_transformer<TransformNode>();
        view->get_transformed_node()->begin_transform_update();
        transform_node->displayed_geometry = animation;

        auto [x, y, width, height] = view->get_geometry();
        transform_node->scale_x = static_cast<float>(animation.width / width);
        transform_node->scale_y = static_cast<float>(animation.height / height);

        transform_node->translation_x = static_cast<float>(animation.x + animation.width / 2 - (x + width / 2.0));
        transform_node->translation_y = static_cast<float>(animation.y + animation.height / 2 - (y + height / 2.0));

        transform_node->overlay_alpha = animation.progress();
        view->get_transformed_node()->end_transform_update();
    };

    signal::connection_t<view_disappeared_signal> on_disappear = [=] (const view_disappeared_signal *ev)
    {
        if (ev->view == view)
            destroy();
    };
};
}


