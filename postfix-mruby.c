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

mrb_value log_verbose(mrb_state *mrb, mrb_value klass)
{
  extern int msg_verbose;
  return mrb_fixnum_value(msg_verbose);
}

mrb_value log_verbose_p(mrb_state *mrb, mrb_value klass)
{
  extern int msg_verbose;
  return msg_verbose ? mrb_true_value() : mrb_false_value();
}

mrb_value log_info(mrb_state *mrb, mrb_value klass)
{
  mrb_value s;
  mrb_get_args(mrb, "S", &s);
  msg_info("%s", mrb_string_value_cstr(mrb, &s));
  return mrb_nil_value();
}

mrb_value log_warn(mrb_state *mrb, mrb_value klass)
{
  mrb_value s;
  mrb_get_args(mrb, "S", &s);
  msg_warn("%s", mrb_string_value_cstr(mrb, &s));
  return mrb_nil_value();
}

mrb_value log_error(mrb_state *mrb, mrb_value klass)
{
  mrb_value s;
  mrb_get_args(mrb, "S", &s);
  msg_error("%s", mrb_string_value_cstr(mrb, &s));
  return mrb_nil_value();
}

mrb_value log_fatal(mrb_state *mrb, mrb_value klass)
{
  mrb_value s;
  mrb_get_args(mrb, "S", &s);
  msg_fatal("%s", mrb_string_value_cstr(mrb, &s));
  return mrb_nil_value();
}

void init_log_class(mrb_state *mrb)
{
  struct RClass *log;

  log = mrb_define_class(mrb, "Log", mrb->object_class);
  mrb_define_class_method(mrb, log, "verbose", log_verbose, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, log, "verbose?", log_verbose_p, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, log, "info", log_info, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, log, "warn", log_warn, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, log, "error", log_error, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, log, "fatal", log_fatal, MRB_ARGS_REQ(1));
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
  init_log_class(mrb);
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
