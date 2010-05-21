/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *  This work is in public domain.
 *
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  If you have questions, contact Nedko Arnaudov <nedko@arnaudov.name> or
 *  ask in #lad channel, FreeNode IRC network.
 *
 *****************************************************************************/

#ifndef LV2_EXTERNAL_UI_H__5AFE09A5_0FB7_47AF_924E_2AF0F8DE8873__INCLUDED
#define LV2_EXTERNAL_UI_H__5AFE09A5_0FB7_47AF_924E_2AF0F8DE8873__INCLUDED

/** UI extension suitable for out-of-process UIs */
#define LV2_EXTERNAL_UI_URI "http://lv2plug.in/ns/extensions/ui#external"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Adjust editor indent */
#endif

/**
 * When LV2_EXTERNAL_UI_URI UI is instantiated, the returned
 * LV2UI_Widget handle must be cast to pointer to struct lv2_external_ui.
 * UI is created in invisible state.
 */
struct lv2_external_ui
{
  /**
   * Host calls this function regulary. UI library implementing the
   * callback may do IPC or redraw the UI.
   *
   * @param _this_ the UI context
   */
  void (* run)(struct lv2_external_ui * _this_);

  /**
   * Host calls this function to make the plugin UI visible.
   *
   * @param _this_ the UI context
   */
  void (* show)(struct lv2_external_ui * _this_);

  /**
   * Host calls this function to make the plugin UI invisible again.
   *
   * @param _this_ the UI context
   */
  void (* hide)(struct lv2_external_ui * _this_);
};

#define LV2_EXTERNAL_UI_RUN(ptr) (ptr)->run(ptr)
#define LV2_EXTERNAL_UI_SHOW(ptr) (ptr)->show(ptr)
#define LV2_EXTERNAL_UI_HIDE(ptr) (ptr)->hide(ptr)

/**
 * On UI instantiation, host must supply LV2_EXTERNAL_UI_URI
 * feature. LV2_Feature::data must be pointer to struct lv2_external_ui_host. */
struct lv2_external_ui_host
{
  /**
   * Callback that plugin UI will call
   * when UI (GUI window) is closed by user.
   * This callback wil; be called during execution of lv2_external_ui::run()
   * (i.e. not from background thread).
   *
   * After this callback is called, UI is defunct. Host must call
   * LV2UI_Descriptor::cleanup(). If host wants to make the UI visible
   * again UI must be reinstantiated.
   *
   * @param controller Host context associated with plugin UI, as
   * supplied to LV2UI_Descriptor::instantiate()
   */
  void (* ui_closed)(LV2UI_Controller controller);

  /**
   * Optional (may be NULL) "user friendly" identifier which the UI
   * may display to allow a user to easily associate this particular
   * UI instance with the correct plugin instance as it is represented
   * by the host (e.g. "track 1" or "channel 4").
   *
   * If supplied by host, the string will be referenced only during
   * LV2UI_Descriptor::instantiate()
   */
  const char * plugin_human_id;
};

#if 0
{ /* Adjust editor indent */
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* #ifndef LV2_EXTERNAL_UI_H__5AFE09A5_0FB7_47AF_924E_2AF0F8DE8873__INCLUDED */
