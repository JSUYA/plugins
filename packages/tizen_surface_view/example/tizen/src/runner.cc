#include "runner.h"

#include <Elementary.h>

#include "generated_plugin_registrant.h"
#include "log.h"

class App : public FlutterApp {
 public:
  int count = 0;

  static void _btn_cb(void *data, Evas_Object *btn, void *ev EINA_UNUSED)

  {
    int *count = (int *)data;
    char buf[252];
    (*count)++;
    sprintf(buf, "%d", (*count));
    elm_object_text_set(btn, buf);
  }

  Evas_Object *add_content(Evas_Object *parent) {
    Evas_Object *background_ = elm_bg_add(parent);
    evas_object_size_hint_weight_set(background_, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_color_set(background_, 255, 255, 255, 255);
    elm_win_resize_object_add(parent, background_);
    Evas_Object *scr = elm_scroller_add(parent);
    elm_scroller_bounce_set(scr, EINA_FALSE, EINA_FALSE);
    elm_scroller_policy_set(scr, ELM_SCROLLER_POLICY_OFF,
                            ELM_SCROLLER_POLICY_AUTO);
    evas_object_size_hint_weight_set(scr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    evas_object_show(scr);
    evas_object_show(background_);
    evas_object_show(parent);
    Evas_Object *box = elm_box_add(parent);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_object_content_set(scr, box);
    elm_win_resize_object_add(parent, scr);
    Evas_Object *boxsub = elm_box_add(box);
    evas_object_size_hint_weight_set(boxsub, EVAS_HINT_EXPAND, EVAS_HINT_FILL);
    evas_object_size_hint_align_set(boxsub, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_box_horizontal_set(boxsub, EINA_TRUE);
    evas_object_show(boxsub);
    elm_box_pack_end(box, boxsub);

    Evas_Object *button;
    button = elm_button_add(boxsub);
    elm_object_text_set(button, "Tizen Button");
    evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(button);
    elm_box_pack_end(boxsub, button);

    Evas_Object *entry = elm_entry_add(boxsub);
    elm_entry_entry_set(entry, "Entry");
    evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_entry_editable_set(entry, EINA_TRUE);
    elm_entry_single_line_set(entry, EINA_TRUE);
    evas_object_show(entry);
    elm_box_pack_end(boxsub, entry);

    Evas_Object *check = elm_check_add(boxsub);
    elm_object_text_set(check, "Tizen CheckBox");
    evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(check, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(check);
    elm_box_pack_end(box, check);

    Evas_Object *button2 = elm_button_add(boxsub);
    elm_object_text_set(button2, "Tizen Button3");
    evas_object_size_hint_weight_set(button2, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(button2, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_smart_callback_add(button2, "clicked", _btn_cb, &count);
    evas_object_show(button2);
    elm_box_pack_end(box, button2);

    return entry;
  }

  bool OnCreate() {
    // renderer_type_ = FlutterRendererType::kEvasGL;

    build_tizen_platform_view_ = [=](Evas_Object *container) {
      add_content(container);
    };

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
