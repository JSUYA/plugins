#include "runner.h"

#include <Elementary.h>
#include <app.h>
#include <app_manager.h>
#include <efl_extension.h>
#include <package_manager.h>

#include "generated_plugin_registrant.h"
#include "log.h"

struct view_info {
  Evas_Object *win;
  Evas_Object *conform;
  Evas_Object *gengrid;
  Elm_Gengrid_Item_Class *itc;
  Eina_List *app_list;
} s_info = {
    .win = NULL,
    .gengrid = NULL,
    .itc = NULL,
    .app_list = NULL,
};

class App : public FlutterApp {
 public:
  int count = 0;

  static void _entry_cb(void *data, Evas_Object *entry, void *ev EINA_UNUSED)

  {
    Evas_Object *win = static_cast<Evas_Object *>(data);

    LOG_ERROR("CJS TIZEN Entry  Evas : %p", evas_object_evas_get(entry));
    elm_object_focus_set(entry, EINA_TRUE);
  }

  static void _btn_cb(void *data, Evas_Object *btn, void *ev EINA_UNUSED)

  {
    Evas_Object *win = static_cast<Evas_Object *>(data);

    LOG_ERROR("CJS TIZEN BUTTON  Evas : %p", evas_object_evas_get(btn));
    elm_object_focus_set(btn, EINA_TRUE);
  }

  static void _btn_options_cb(void *data, Evas_Object *btn,
                              void *ev EINA_UNUSED)

  {
    elm_object_text_set(btn, "TEST");
    LOG_ERROR("CJS TIZEN BUTTON  ");
  }

  static void _btn_options2_cb(void *data, Evas_Object *btn,
                               void *ev EINA_UNUSED)

  {
    int *count = (int *)data;
    (*count)++;
    char buf[255];
    sprintf(buf, "%d", (*count));
    elm_object_text_set(btn, buf);
    LOG_ERROR("CJS TIZEN BUTTON PRESSED!!!!!!!!!!! ");
  }

  static void key_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                          void *event_info EINA_UNUSED) {
    LOG_ERROR("CJS Down!!!!!!!!!!!!!");
    elm_object_focus_set(obj, EINA_TRUE);
  }

  static void key_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                        void *event_info EINA_UNUSED) {
    LOG_ERROR("CJS Up!!!!!!!!!");
    elm_object_focus_set(obj, EINA_TRUE);
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
    elm_win_resize_object_add(parent, scr);
    evas_object_show(scr);
    evas_object_show(background_);
    evas_object_show(parent);
    Evas_Object *box = elm_box_add(parent);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_object_content_set(scr, box);
    Evas_Object *boxsub = elm_box_add(box);
    evas_object_size_hint_weight_set(boxsub, EVAS_HINT_EXPAND, EVAS_HINT_FILL);
    evas_object_size_hint_align_set(boxsub, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_box_horizontal_set(boxsub, EINA_TRUE);
    evas_object_show(boxsub);
    elm_box_pack_end(box, boxsub);

    Evas_Object *entry = elm_entry_add(boxsub);
    elm_entry_entry_set(entry, "Entry");
    evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_entry_editable_set(entry, EINA_TRUE);
    elm_entry_single_line_set(entry, EINA_TRUE);
    evas_object_smart_callback_add(entry, "clicked", _entry_cb, parent);
    // elm_entry_input_panel_enabled_set(entry, EINA_TRUE);
    evas_object_show(entry);
    elm_box_pack_end(boxsub, entry);

    Evas_Object *button;
    button = elm_button_add(boxsub);
    elm_object_text_set(button, "Tizen Button");
    elm_object_focus_allow_set(button, EINA_TRUE);
    evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_smart_callback_add(button, "clicked", _btn_cb, parent);
    evas_object_show(button);
    elm_box_pack_end(boxsub, button);

    Evas_Object *button2 = elm_check_add(boxsub);
    elm_object_text_set(button2, "Tizen CheckBox");
    evas_object_size_hint_weight_set(button2, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(button2, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_smart_callback_add(button2, "clicked", _btn_options_cb, &count);
    evas_object_show(button2);
    elm_box_pack_end(box, button2);

    Evas_Object *button3 = elm_button_add(boxsub);
    elm_object_text_set(button3, "Tizen Button3");
    evas_object_size_hint_weight_set(button3, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(button3, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(button3);
    elm_box_pack_end(box, button3);

    evas_object_smart_callback_add(button3, "clicked", _btn_options2_cb,
                                   &count);

    // SimpleHome code
    Eina_List *it = NULL;
    app_info_h *app_info = NULL;
    s_info.win = parent;
    s_info.gengrid = _create_gengrid(s_info.win);
    s_info.app_list = get_all_apps();
    for (it = s_info.app_list, app_info = (app_info_h *)eina_list_data_get(it);
         it; it = eina_list_next(it),
        app_info = (app_info_h *)eina_list_data_get(it)) {
      _append_item(s_info.gengrid, app_info);
    }
    elm_box_pack_end(box, s_info.gengrid);

    return button2;
  }

  bool OnCreate() {
    renderer_type_ = FlutterRendererType::kEvasGL;

    create_tizen_view_ = [=](Evas_Object *win) {
      add_content(win);
      evas_object_event_callback_add(win, EVAS_CALLBACK_KEY_DOWN, key_down_cb,
                                     this);
      evas_object_event_callback_add(win, EVAS_CALLBACK_KEY_UP, key_up_cb,
                                     this);
      LOG_ERROR("CJS surface_win %p %p", win, evas_object_evas_get(win));
    };

    if (FlutterApp::OnCreate()) {
      RegisterPlugins(this);
    }
    return IsRunning();
  }

  //////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////

#define BUF_SIZE 1024
#define POPUP_TIMEOUT 5.0
#define GENGRID_ITEM_WIDTH 70
#define GENGRID_ITEM_HEIGHT 70

  static void _delete_win_request_cb(void *data, Evas_Object *obj,
                                     void *event_info) {
    ui_app_exit();
  }

  static void _win_back_cb(void *data, Evas_Object *obj, void *event_info) {
    elm_win_lower(s_info.win);
  }

  static void _block_clicked_cb(void *data, Evas_Object *obj,
                                void *event_info) {
    evas_object_del(obj);
  }

  static void _gengrid_item_selected_cb(void *data, Evas_Object *obj,
                                        void *event_info) {
    _create_popup(obj, (app_info_h *)data);
  }

  static Evas_Object *_get_grid_content_cb(void *data, Evas_Object *obj,
                                           const char *part) {
    Evas_Object *image;
    Evas_Object *bg;
    char *icon;
    app_info_h *app_info = (app_info_h *)data;

    if (!strcmp(part, "elm.swallow.icon")) {
      image = elm_image_add(obj);
      app_info_get_icon(*app_info, &icon);
      if (!elm_image_file_set(image, icon, NULL)) {
        evas_object_del(image);
        bg = elm_bg_add(obj);
        elm_bg_color_set(bg, 255, 255, 0);
        return bg;
      }

      return image;
    }

    return NULL;
  }

  static void _create_popup(Evas_Object *parent, app_info_h *app_info) {
    char *label;
    char *icon;
    char *package;
    char info[BUF_SIZE];
    Evas_Object *popup = elm_popup_add(parent);

    get_app_label(app_info, &label);
    get_app_icon(app_info, &icon);
    get_app_package(app_info, &package);

    elm_object_part_text_set(popup, "title,text", label);

    snprintf(info, BUF_SIZE, "Icon path:<br>%s<br><br>Package:<br>%s", icon,
             package);
    elm_object_part_text_set(popup, "default", info);

    free(label);
    free(icon);
    free(package);

    evas_object_smart_callback_add(popup, "block,clicked", _block_clicked_cb,
                                   NULL);
    elm_popup_timeout_set(popup, POPUP_TIMEOUT);

    evas_object_show(popup);
  }

  static Evas_Object *_create_gengrid(Evas_Object *parent) {
    Evas_Object *gengrid;

    if (!s_info.itc) _initialize_gengrid_item_class();

    gengrid = elm_gengrid_add(parent);
    elm_gengrid_item_size_set(gengrid, GENGRID_ITEM_WIDTH, GENGRID_ITEM_HEIGHT);
    elm_gengrid_reorder_mode_set(gengrid, EINA_TRUE);
    elm_gengrid_horizontal_set(gengrid, EINA_TRUE);
    evas_object_size_hint_weight_set(gengrid, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(gengrid, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_min_set(gengrid, 315, 200);
    evas_object_size_hint_max_set(gengrid, 315, 200);
    evas_object_show(gengrid);

    return gengrid;
  }

  static Elm_Object_Item *_append_item(Evas_Object *gengrid,
                                       app_info_h *app_info) {
    Elm_Object_Item *item;

    if (!s_info.itc) _initialize_gengrid_item_class();

    item = elm_gengrid_item_append(gengrid, s_info.itc, app_info,
                                   _gengrid_item_selected_cb, app_info);
    return item;
  }

  static void _initialize_gengrid_item_class(void) {
    s_info.itc = elm_gengrid_item_class_new();
    s_info.itc->item_style = "default";
    s_info.itc->func.text_get = NULL;
    s_info.itc->func.content_get = _get_grid_content_cb;
    s_info.itc->func.state_get = NULL;
    s_info.itc->func.del = NULL;
  }

  Eina_List *get_all_apps(void) {
    Eina_List *app_list = NULL;
    app_manager_foreach_app_info(__app_manager_app_info_cb, &app_list);
    return app_list;
  }

  static bool get_app_label(app_info_h *app_info, char **label) {
    return __is_app_successful(
        (app_manager_error_e)app_info_get_label(*app_info, label));
  }

  static bool get_app_icon(app_info_h *app_info, char **path) {
    return __is_app_successful(
        (app_manager_error_e)app_info_get_icon(*app_info, path));
  }

  static bool get_app_package(app_info_h *app_info, char **package) {
    return __is_app_successful(
        (app_manager_error_e)app_info_get_package(*app_info, package));
  }

  static bool __is_app_successful(app_manager_error_e err_code) {
    if (err_code == APP_MANAGER_ERROR_NONE) return true;

    return false;
  }

  static bool __app_manager_app_info_cb(app_info_h app_info, void *user_data) {
    bool nodisplay;
    __is_app_successful(
        (app_manager_error_e)app_info_is_nodisplay(app_info, &nodisplay));
    if (!nodisplay) {
      Eina_List **app_list = (Eina_List **)user_data;
      app_info_h *clone = (app_info_h *)malloc(sizeof(app_info_h));
      __is_app_successful((app_manager_error_e)app_info_clone(clone, app_info));
      *app_list = eina_list_append(*app_list, clone);
    }
    return true;
  }
};

int main(int argc, char *argv[]) {
  App app;
  return app.Run(argc, argv);
}
