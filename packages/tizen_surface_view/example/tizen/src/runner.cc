#include "runner.h"

#include "generated_plugin_registrant.h"

class App : public FlutterApp {
 public:
  static void _btn_elm_clicked(void *data, Evas_Object *obj EINA_UNUSED,
                               void *event_info EINA_UNUSED) {
    Evas_Object *win = (Evas_Object *)data;
  }

  static void _btn_options_cb(void *data, Evas_Object *btn,
                              void *ev EINA_UNUSED)

  {
    elm_object_text_set(btn, "22222222222");
  }

  static void _btn_options_cb2(void *data, Evas_Object *btn,
                               void *ev EINA_UNUSED)

  {
    Evas_Object *btn2 = (Evas_Object *)data;
    elm_object_text_set(btn2, "22222222222");
  }

  Evas_Object *contents(Evas_Object *platformview_window_) {
    Evas_Object *background_ = elm_bg_add(platformview_window_);
    evas_object_size_hint_weight_set(background_, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_color_set(background_, 255, 128, 0, 200);
    elm_win_resize_object_add(platformview_window_, background_);
    Evas_Object *scr = elm_scroller_add(platformview_window_);
    elm_scroller_bounce_set(scr, EINA_FALSE, EINA_FALSE);
    elm_scroller_policy_set(scr, ELM_SCROLLER_POLICY_OFF,
                            ELM_SCROLLER_POLICY_AUTO);
    evas_object_size_hint_weight_set(scr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(platformview_window_, scr);
    evas_object_show(scr);
    evas_object_show(background_);
    evas_object_show(platformview_window_);
    Evas_Object *box = elm_box_add(platformview_window_);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_object_content_set(scr, box);
    Evas_Object *boxsub = elm_box_add(box);
    evas_object_size_hint_weight_set(boxsub, EVAS_HINT_EXPAND, EVAS_HINT_FILL);
    evas_object_size_hint_align_set(boxsub, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_box_horizontal_set(boxsub, EINA_TRUE);
    evas_object_show(boxsub);
    elm_box_pack_end(box, boxsub);
    Evas_Object *button;
    /*button = elm_button_add(boxsub);
    elm_object_text_set(button, "Tizen Button");
    evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(button);
    elm_box_pack_end(boxsub, button);
    button = elm_button_add(boxsub);
    elm_object_text_set(button, "Tizen Button");
    evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(button);
    elm_box_pack_end(box, button);

    evas_object_smart_callback_add(button, "clicked", _btn_options_cb, nullptr);
*/
    button = elm_button_add(boxsub);
    elm_object_text_set(button, "Tizen Button");
    evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(button);
    elm_box_pack_end(box, button);
    evas_object_smart_callback_add(button, "clicked", _btn_options_cb, nullptr);
    return button;
  }

  bool OnCreate() {
    // renderer_type_ = FlutterRendererType::kEvasGL;

    /*  Evas_Object *window_ = elm_win_add(NULL, NULL, ELM_WIN_BASIC);
      elm_win_autodel_set(window_, EINA_TRUE);
      elm_win_alpha_set(window_, EINA_TRUE);
      contents(window_);
  */
    // window_width_ = 700;
    // window_height_ = 700;

    Ecore_Evas *ee = ecore_evas_buffer_new(315, 505);
    Evas_Object *surface_win = elm_win_fake_add(ee);
    // Evas_Object *image = ecore_evas_object_image_new(ee);
    Evas_Object *btn = contents(surface_win);
    evas_object_smart_callback_add(surface_win, "clicked", _btn_options_cb2,
                                   btn);

    platformview_window_ = btn;

    if (FlutterApp::OnCreate()) {
      RegisterPlugins(this);
    }
    return IsRunning();
  }
};

int main(int argc, char *argv[]) {
  App app;
  return app.Run(argc, argv);
}
