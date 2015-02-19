/*
 * See https://github.com/tmtm/postfix-mruby
 *
 * Copyright (c) 2015 TOMITA Masahiro <tommy@tmtm.org>
 */
#include <stdio.h>

#include "sys_defs.h"
#include "dict.h"

#include "mruby.h"
#include "mruby/compile.h"
#include "mruby/string.h"

#define DICT_TYPE_MRUBY "mruby"

typedef struct {
  DICT dict;
  mrb_state *mrb;
  mrb_value obj;
} DICT_MRUBY;

static const char *dict_mruby_lookup(DICT *dict, const char *name)
{
  mrb_value val, str;
  mrb_state *mrb = ((DICT_MRUBY *)dict)->mrb;
  mrb_value obj = ((DICT_MRUBY *)dict)->obj;

  val = mrb_funcall(mrb, obj, "lookup", 1, mrb_str_new_cstr(mrb, name));
  if (mrb->exc) {
    mrb_value s = mrb_funcall(mrb, mrb_obj_value(mrb->exc), "inspect", 0);
    msg_warn("dict_mruby_lookup: %s", mrb_string_value_cstr(mrb, &s));
    dict->error = DICT_ERR_CONFIG;
    return NULL;
  }
  str = mrb_funcall(mrb, val, "to_s", 0);
  return mrb_string_value_cstr(mrb, &str);
}

static void dict_mruby_close(DICT *dict)
{
  mrb_close(((DICT_MRUBY *)dict)->mrb);
  dict_free(dict);
}

DICT *dict_mruby_open(const char *path, int open_flags, int dict_flags)
{
  DICT_MRUBY *dict;
  mrb_state *mrb;
  mrb_value obj;
  FILE *f;

  if ((f = fopen(path, "r")) == NULL)
    return dict_surrogate(DICT_TYPE_MRUBY, path, open_flags, dict_flags, "open file %s: %m", path);
  mrb = mrb_open();
  obj = mrb_load_file(mrb, f);
  fclose(f);

  if (mrb->exc) {
    mrb_value s = mrb_funcall(mrb, mrb_obj_value(mrb->exc), "inspect", 0);
    return dict_surrogate(DICT_TYPE_MRUBY, path, open_flags, dict_flags, "load file %s: %s", path, mrb_string_value_cstr(mrb, &s));
  }

  dict = (DICT_MRUBY *)dict_alloc(DICT_TYPE_MRUBY, path, sizeof(DICT_MRUBY));
  dict->dict.lookup = dict_mruby_lookup;
  dict->dict.close = dict_mruby_close;
  dict->dict.flags = dict_flags;
  dict->mrb = mrb;
  dict->obj = obj;

  return (DICT *)dict;
}
