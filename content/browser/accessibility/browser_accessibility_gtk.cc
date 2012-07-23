// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/browser_accessibility_gtk.h"

#include <gtk/gtk.h>

#include "base/utf_string_conversions.h"
#include "content/browser/accessibility/browser_accessibility_manager_gtk.h"
#include "content/common/accessibility_messages.h"

using content::AccessibilityNodeData;

// The maximum length of an autogenerated unique type name string,
// generated from the 16-bit interface mask from an AtkObject.
// 30 is enough for the prefix "WAIType" + 5 hex chars (max) */
static const int kWAITypeNameLen = 30;

static gpointer browser_accessibility_parent_class = NULL;

static BrowserAccessibilityGtk* ToBrowserAccessibilityGtk(
    BrowserAccessibilityAtk* atk_object) {
  if (!atk_object)
    return NULL;

  return atk_object->m_object;
}

static BrowserAccessibilityGtk* ToBrowserAccessibilityGtk(
    AtkObject* atk_object) {
  if (!IS_BROWSER_ACCESSIBILITY(atk_object))
    return NULL;

  return ToBrowserAccessibilityGtk(BROWSER_ACCESSIBILITY(atk_object));
}

static const gchar* browser_accessibility_get_name(AtkObject* atk_object) {
  BrowserAccessibilityGtk* obj = ToBrowserAccessibilityGtk(atk_object);
  if (!obj)
    return NULL;
  return obj->atk_acc_name().c_str();
}

static const gchar* browser_accessibility_get_description(
    AtkObject* atk_object) {
  BrowserAccessibilityGtk* obj = ToBrowserAccessibilityGtk(atk_object);
  if (!obj)
    return NULL;
  return obj->atk_acc_description().c_str();
}

static AtkObject* browser_accessibility_get_parent(AtkObject* atk_object) {
  BrowserAccessibilityGtk* obj = ToBrowserAccessibilityGtk(atk_object);
  if (!obj)
    return NULL;
  if (obj->parent())
    return obj->parent()->ToBrowserAccessibilityGtk()->GetAtkObject();
  else
    return gtk_widget_get_accessible(obj->manager()->GetParentView());
}

static gint browser_accessibility_get_n_children(AtkObject* atk_object) {
  BrowserAccessibilityGtk* obj = ToBrowserAccessibilityGtk(atk_object);
  if (!obj)
    return 0;
  return obj->children().size();
}

static AtkObject* browser_accessibility_ref_child(
    AtkObject* atk_object, gint index) {
  BrowserAccessibilityGtk* obj = ToBrowserAccessibilityGtk(atk_object);
  if (!obj)
    return NULL;
  AtkObject* result =
      obj->children()[index]->ToBrowserAccessibilityGtk()->GetAtkObject();
  g_object_ref(result);
  return result;
}

static gint browser_accessibility_get_index_in_parent(AtkObject* atk_object) {
  BrowserAccessibilityGtk* obj = ToBrowserAccessibilityGtk(atk_object);
  if (!obj)
    return 0;
  return obj->index_in_parent();
}

static AtkAttributeSet* browser_accessibility_get_attributes(
    AtkObject* atk_object) {
  return NULL;
}

static AtkRole browser_accessibility_get_role(AtkObject* atk_object) {
  BrowserAccessibilityGtk* obj = ToBrowserAccessibilityGtk(atk_object);
  if (!obj)
    return ATK_ROLE_INVALID;
  return obj->atk_role();
}

static AtkStateSet* browser_accessibility_ref_state_set(AtkObject* atk_object) {
  BrowserAccessibilityGtk* obj = ToBrowserAccessibilityGtk(atk_object);
  if (!obj)
    return NULL;
  AtkStateSet* state_set =
      ATK_OBJECT_CLASS(browser_accessibility_parent_class)->
          ref_state_set(atk_object);
  int32 state = obj->state();

  if ((state >> AccessibilityNodeData::STATE_FOCUSABLE) & 1)
    atk_state_set_add_state(state_set, ATK_STATE_FOCUSABLE);
  if (obj->manager()->GetFocus(NULL) == obj)
    atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);

  return state_set;
}

static AtkRelationSet* browser_accessibility_ref_relation_set(
    AtkObject* atk_object) {
  AtkRelationSet* relation_set =
      ATK_OBJECT_CLASS(browser_accessibility_parent_class)
          ->ref_relation_set(atk_object);
  return relation_set;
}

static void browser_accessibility_init(AtkObject* atk_object, gpointer data) {
  if (ATK_OBJECT_CLASS(browser_accessibility_parent_class)->initialize) {
    ATK_OBJECT_CLASS(browser_accessibility_parent_class)->initialize(
        atk_object, data);
  }

  BROWSER_ACCESSIBILITY(atk_object)->m_object =
      reinterpret_cast<BrowserAccessibilityGtk*>(data);
}

static void browser_accessibility_finalize(GObject* atk_object) {
  G_OBJECT_CLASS(browser_accessibility_parent_class)->finalize(atk_object);
}

static void browser_accessibility_class_init(AtkObjectClass* klass) {
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  browser_accessibility_parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = browser_accessibility_finalize;
  klass->initialize = browser_accessibility_init;
  klass->get_name = browser_accessibility_get_name;
  klass->get_description = browser_accessibility_get_description;
  klass->get_parent = browser_accessibility_get_parent;
  klass->get_n_children = browser_accessibility_get_n_children;
  klass->ref_child = browser_accessibility_ref_child;
  klass->get_role = browser_accessibility_get_role;
  klass->ref_state_set = browser_accessibility_ref_state_set;
  klass->get_index_in_parent = browser_accessibility_get_index_in_parent;
  klass->get_attributes = browser_accessibility_get_attributes;
  klass->ref_relation_set = browser_accessibility_ref_relation_set;
}

GType browser_accessibility_get_type() {
  static volatile gsize type_volatile = 0;

  if (g_once_init_enter(&type_volatile)) {
    static const GTypeInfo tinfo = {
      sizeof(BrowserAccessibilityAtkClass),
      (GBaseInitFunc) 0,
      (GBaseFinalizeFunc) 0,
      (GClassInitFunc) browser_accessibility_class_init,
      (GClassFinalizeFunc) 0,
      0, /* class data */
      sizeof(BrowserAccessibilityAtk), /* instance size */
      0, /* nb preallocs */
      (GInstanceInitFunc) 0,
      0 /* value table */
    };

    GType type = g_type_register_static(
        ATK_TYPE_OBJECT, "BrowserAccessibility", &tinfo, GTypeFlags(0));
    g_once_init_leave(&type_volatile, type);
  }

  return type_volatile;
}

static guint16 GetInterfaceMaskFromObject(BrowserAccessibilityGtk* obj) {
  return 0;
}

static const char* GetUniqueAccessibilityTypeName(guint16 interface_mask)
{
  static char name[kWAITypeNameLen + 1];

  sprintf(name, "WAIType%x", interface_mask);
  name[kWAITypeNameLen] = '\0';

  return name;
}

static const GInterfaceInfo AtkInterfacesInitFunctions[] = {
};

enum WAIType {
  WAI_ACTION,
  WAI_SELECTION,
  WAI_EDITABLE_TEXT,
  WAI_TEXT,
  WAI_COMPONENT,
  WAI_IMAGE,
  WAI_TABLE,
  WAI_HYPERTEXT,
  WAI_HYPERLINK,
  WAI_DOCUMENT,
  WAI_VALUE,
};

static GType GetAtkInterfaceTypeFromWAIType(WAIType type) {
  switch (type) {
    case WAI_ACTION:
      return ATK_TYPE_ACTION;
    case WAI_SELECTION:
      return ATK_TYPE_SELECTION;
    case WAI_EDITABLE_TEXT:
      return ATK_TYPE_EDITABLE_TEXT;
    case WAI_TEXT:
      return ATK_TYPE_TEXT;
    case WAI_COMPONENT:
      return ATK_TYPE_COMPONENT;
    case WAI_IMAGE:
      return ATK_TYPE_IMAGE;
    case WAI_TABLE:
      return ATK_TYPE_TABLE;
    case WAI_HYPERTEXT:
      return ATK_TYPE_HYPERTEXT;
    case WAI_HYPERLINK:
      return ATK_TYPE_HYPERLINK_IMPL;
    case WAI_DOCUMENT:
      return ATK_TYPE_DOCUMENT;
    case WAI_VALUE:
      return ATK_TYPE_VALUE;
  }

  return G_TYPE_INVALID;
}

static GType GetAccessibilityTypeFromObject(BrowserAccessibilityGtk* obj) {
  static const GTypeInfo type_info = {
    sizeof(BrowserAccessibilityAtkClass),
    (GBaseInitFunc) 0,
    (GBaseFinalizeFunc) 0,
    (GClassInitFunc) 0,
    (GClassFinalizeFunc) 0,
    0, /* class data */
    sizeof(BrowserAccessibilityAtk), /* instance size */
    0, /* nb preallocs */
    (GInstanceInitFunc) 0,
    0 /* value table */
  };

  guint16 interface_mask = GetInterfaceMaskFromObject(obj);
  const char* atk_type_name = GetUniqueAccessibilityTypeName(interface_mask);
  GType type = g_type_from_name(atk_type_name);
  if (type)
    return type;

  type = g_type_register_static(BROWSER_ACCESSIBILITY_TYPE,
                                atk_type_name,
                                &type_info,
                                GTypeFlags(0));
  for (guint i = 0; i < G_N_ELEMENTS(AtkInterfacesInitFunctions); i++) {
    if (interface_mask & (1 << i)) {
      g_type_add_interface_static(
          type,
          GetAtkInterfaceTypeFromWAIType(static_cast<WAIType>(i)),
          &AtkInterfacesInitFunctions[i]);
    }
  }

  return type;
}

BrowserAccessibilityAtk* browser_accessibility_new(
    BrowserAccessibilityGtk* obj) {
  GType type = GetAccessibilityTypeFromObject(obj);
  AtkObject* atk_object = static_cast<AtkObject*>(g_object_new(type, 0));

  atk_object_initialize(atk_object, obj);

  return BROWSER_ACCESSIBILITY(atk_object);
}

void browser_accessibility_detach(BrowserAccessibilityAtk* atk_object) {
  atk_object->m_object = NULL;
}

// static
BrowserAccessibility* BrowserAccessibility::Create() {
  return new BrowserAccessibilityGtk();
}

BrowserAccessibilityGtk* BrowserAccessibility::ToBrowserAccessibilityGtk() {
  return static_cast<BrowserAccessibilityGtk*>(this);
}

BrowserAccessibilityGtk::BrowserAccessibilityGtk() {
  atk_object_ = ATK_OBJECT(browser_accessibility_new(this));
}

BrowserAccessibilityGtk::~BrowserAccessibilityGtk() {
  browser_accessibility_detach(BROWSER_ACCESSIBILITY(atk_object_));
  g_object_unref(atk_object_);
}

AtkObject* BrowserAccessibilityGtk::GetAtkObject() const {
  if (!G_IS_OBJECT(atk_object_))
    return NULL;
  return atk_object_;
}

void BrowserAccessibilityGtk::PreInitialize() {
  BrowserAccessibility::PreInitialize();
  InitRoleAndState();

  if (this->parent()) {
    atk_object_set_parent(
        atk_object_,
        this->parent()->ToBrowserAccessibilityGtk()->GetAtkObject());
  }
}

bool BrowserAccessibilityGtk::IsNative() const {
  return true;
}

void BrowserAccessibilityGtk::InitRoleAndState() {
  atk_acc_name_ = UTF16ToUTF8(name());

  string16 description;
  GetStringAttribute(AccessibilityNodeData::ATTR_DESCRIPTION, &description);
  atk_acc_description_ = UTF16ToUTF8(description);

  switch(role_) {
    case AccessibilityNodeData::ROLE_BUTTON:
      atk_role_ = ATK_ROLE_PUSH_BUTTON;
      break;
    case AccessibilityNodeData::ROLE_CHECKBOX:
      atk_role_ = ATK_ROLE_CHECK_BOX;
      break;
    case AccessibilityNodeData::ROLE_COMBO_BOX:
      atk_role_ = ATK_ROLE_COMBO_BOX;
      break;
    case AccessibilityNodeData::ROLE_LINK:
      atk_role_ = ATK_ROLE_LINK;
      break;
    case AccessibilityNodeData::ROLE_RADIO_BUTTON:
      atk_role_ = ATK_ROLE_RADIO_BUTTON;
      break;
    case AccessibilityNodeData::ROLE_TEXTAREA:
      atk_role_ = ATK_ROLE_ENTRY;
      break;
    case AccessibilityNodeData::ROLE_TEXT_FIELD:
      atk_role_ = ATK_ROLE_ENTRY;
      break;
    case AccessibilityNodeData::ROLE_WEBCORE_LINK:
      atk_role_ = ATK_ROLE_LINK;
      break;
    default:
      atk_role_ = ATK_ROLE_UNKNOWN;
      break;
  }
}
