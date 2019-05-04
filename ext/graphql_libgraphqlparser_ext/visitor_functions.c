#include "graphql_libgraphqlparser_ext.h"

// Get the name from `node` using `get_name_fn`,
// then assign it to `assigns` hash with key `:name`
#define ASSIGN_NAME(assigns, node, get_name_fn) \
  rb_hash_aset(assigns, name_sym,    \
    rb_str_new2(                                \
      GraphQLAstName_get_value(                 \
          get_name_fn(node)                     \
        )                                       \
      )                                         \
    );                                          \

VALUE name_sym, add_value_intern, end_visit_intern, begin_visit_intern, document_end_visit_intern;
VALUE Libgraphqlparser_PositionSource;

int rb_utf_8_enc;

// Get the line & column from `node` and assign it to `rb_node`
inline VALUE get_position_source(const struct GraphQLAstNode *node) {
  struct GraphQLAstLocation location = {0};
  graphql_node_get_location(node, &location);
  VALUE args[2];
  args[0] = INT2NUM(location.beginLine);
  args[1] = INT2NUM(location.beginColumn);
  return rb_class_new_instance(2, args, Libgraphqlparser_PositionSource);
}

// Call the finalizer method on `builder_ptr`
inline void end_visit(void * builder_ptr) {
  rb_funcall(
    (VALUE) builder_ptr,
    end_visit_intern,
    0
  );
}

// Build a Ruby node named `node_name_string` out of `node` and return it
void build_rb_node(struct GraphQLAstNode* node, char* node_name_string, void* builder_ptr, VALUE assigns) {
  if (assigns == Qnil) {
    assigns = rb_hash_new();
  }
  VALUE position_source = get_position_source(node);
  rb_hash_aset(assigns, ID2SYM(rb_intern("position_source")), position_source);

  rb_funcall(
      (VALUE) builder_ptr,
      begin_visit_intern,
      2,
      rb_str_new2(node_name_string),
      assigns
    );

  return;
}

// Send `rb_literal_value` to the current node's `#value=` method
inline void assign_literal_value(VALUE rb_literal_value, void* builder_ptr) {
  rb_funcall(
      (VALUE) builder_ptr,
      add_value_intern,
      1,
      rb_literal_value
  );
}

// Prepare a bunch of global Ruby method IDs
void init_visitor_functions(VALUE Libgraphqlparser) {
  name_sym = ID2SYM(rb_intern("name"));
  add_value_intern = rb_intern("add_value");
  end_visit_intern = rb_intern("end_visit");
  begin_visit_intern = rb_intern("begin_visit");
  document_end_visit_intern = rb_intern("document_end_visit");
  rb_utf_8_enc = rb_enc_find_index("UTF-8");
  Libgraphqlparser_PositionSource = rb_const_get(Libgraphqlparser, rb_intern("PositionSource"));
}

// There's a `begin_visit` and `end_visit` for each node.
// Some of the end_visit callbacks are empty but that's ok,
// It lets us use macros in the other files.

int document_begin_visit(const struct GraphQLAstDocument* node, void* builder_ptr) {
  build_rb_node((struct GraphQLAstNode*) node, "Document", builder_ptr, Qnil);
  return 1;
}

void document_end_visit(const struct GraphQLAstDocument* node, void* builder_ptr) {
  rb_funcall((VALUE) builder_ptr, document_end_visit_intern, 0);
}

int operation_definition_begin_visit(const struct GraphQLAstOperationDefinition* node, void* builder_ptr) {
  const struct GraphQLAstName* ast_operation_name;
  const char* operation_type;
  VALUE operation_type_str;
  VALUE assigns = rb_hash_new();

  ast_operation_name = GraphQLAstOperationDefinition_get_name(node);
  if (ast_operation_name) {
    const char* operation_name = GraphQLAstName_get_value(ast_operation_name);
    rb_hash_aset(assigns, name_sym, rb_str_new2(operation_name));
  }

  operation_type = GraphQLAstOperationDefinition_get_operation(node);

  if (operation_type) {
    operation_type_str = rb_str_new2(operation_type);
  } else {
    operation_type_str = rb_str_new2("query");
  }

  rb_hash_aset(assigns, ID2SYM(rb_intern("operation_type")), operation_type_str);

  build_rb_node((struct GraphQLAstNode*) node, "OperationDefinition", builder_ptr, assigns);
  return 1;
}

void operation_definition_end_visit(const struct GraphQLAstOperationDefinition* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int variable_definition_begin_visit(const struct GraphQLAstVariableDefinition* node, void* builder_ptr) {
  build_rb_node((struct GraphQLAstNode*) node, "VariableDefinition", builder_ptr, Qnil);
  return 1;
}

void variable_definition_end_visit(const struct GraphQLAstVariableDefinition* node, void* builder_ptr) {
  end_visit(builder_ptr);
}


int fragment_definition_begin_visit(const struct GraphQLAstFragmentDefinition* node, void* builder_ptr) {
  VALUE assigns = rb_hash_new();
  ASSIGN_NAME(assigns, node, GraphQLAstFragmentDefinition_get_name)
  build_rb_node((struct GraphQLAstNode*) node, "FragmentDefinition", builder_ptr, assigns);
  return 1;
}

void fragment_definition_end_visit(const struct GraphQLAstFragmentDefinition* node, void* builder_ptr) {
  end_visit(builder_ptr);
}


int variable_begin_visit(const struct GraphQLAstVariable* node, void* builder_ptr) {
  VALUE assigns = rb_hash_new();
  // This might actually assign the name of a VariableDefinition:
  ASSIGN_NAME(assigns, node, GraphQLAstVariable_get_name)

  build_rb_node((struct GraphQLAstNode*) node, "VariableIdentifier", builder_ptr, assigns);
  return 1;
}

void variable_end_visit(const struct GraphQLAstVariable* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int field_begin_visit(const struct GraphQLAstField* node, void* builder_ptr) {
  VALUE assigns = rb_hash_new();
  const struct GraphQLAstName* ast_field_alias;
  const char* str_field_alias;

  ASSIGN_NAME(assigns, node, GraphQLAstField_get_name)

  ast_field_alias = GraphQLAstField_get_alias(node);
  if (ast_field_alias) {
    str_field_alias = GraphQLAstName_get_value(ast_field_alias);
    rb_hash_aset(assigns, ID2SYM(rb_intern("alias")), rb_str_new2(str_field_alias));
  }

  build_rb_node((struct GraphQLAstNode*) node, "Field", builder_ptr, assigns);
  return 1;
}

void field_end_visit(const struct GraphQLAstField* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int directive_begin_visit(const struct GraphQLAstDirective* node, void* builder_ptr) {
  VALUE assigns = rb_hash_new();
  ASSIGN_NAME(assigns, node, GraphQLAstDirective_get_name)
  build_rb_node((struct GraphQLAstNode*) node, "Directive", builder_ptr, assigns);

  return 1;
}

void directive_end_visit(const struct GraphQLAstDirective* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int argument_begin_visit(const struct GraphQLAstArgument* node, void* builder_ptr) {
  VALUE assigns = rb_hash_new();
  ASSIGN_NAME(assigns, node, GraphQLAstArgument_get_name)
  build_rb_node((struct GraphQLAstNode*) node, "Argument", builder_ptr, assigns);
  return 1;
}

void argument_end_visit(const struct GraphQLAstArgument* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int fragment_spread_begin_visit(const struct GraphQLAstFragmentSpread* node, void* builder_ptr) {
  VALUE assigns = rb_hash_new();
  ASSIGN_NAME(assigns, node, GraphQLAstFragmentSpread_get_name)
  build_rb_node((struct GraphQLAstNode*) node, "FragmentSpread", builder_ptr, assigns);
  return 1;
}

void fragment_spread_end_visit(const struct GraphQLAstFragmentSpread* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int inline_fragment_begin_visit(const struct GraphQLAstInlineFragment* node, void* builder_ptr) {
  build_rb_node((struct GraphQLAstNode*) node, "InlineFragment", builder_ptr, Qnil);
  return 1;
}

void inline_fragment_end_visit(const struct GraphQLAstInlineFragment* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int list_type_begin_visit(const struct GraphQLAstListType* node, void* builder_ptr) {
  build_rb_node((struct GraphQLAstNode*) node, "ListType", builder_ptr, Qnil);
  return 1;
}

void list_type_end_visit(const struct GraphQLAstListType* node, void* builder_ptr) {
  end_visit(builder_ptr);
}


int non_null_type_begin_visit(const struct GraphQLAstNonNullType* node, void* builder_ptr) {
  build_rb_node((struct GraphQLAstNode*) node, "NonNullType", builder_ptr, Qnil);
  return 1;
}

void non_null_type_end_visit(const struct GraphQLAstNonNullType* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int named_type_begin_visit(const struct GraphQLAstNamedType* node, void* builder_ptr) {
  VALUE assigns = rb_hash_new();
  ASSIGN_NAME(assigns, node, GraphQLAstNamedType_get_name)
  build_rb_node((struct GraphQLAstNode*) node, "TypeName", builder_ptr, assigns);
  return 1;
}

void named_type_end_visit(const struct GraphQLAstNamedType* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int null_value_begin_visit(const struct GraphQLAstNullValue* node, void* builder_ptr) {
  VALUE assigns = rb_hash_new();
  VALUE rb_string = rb_str_new2("null");
  rb_hash_aset(assigns, name_sym, rb_string);
  build_rb_node((struct GraphQLAstNode*) node, "NullValue", builder_ptr, assigns);
  return 1;
}

void null_value_end_visit(const struct GraphQLAstListType* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int float_value_begin_visit(const struct GraphQLAstFloatValue* node, void* builder_ptr) {
  const char* str_float = GraphQLAstFloatValue_get_value(node);
  VALUE rb_float = rb_funcall(rb_str_new2(str_float), rb_intern("to_f"), 0);
  assign_literal_value(rb_float, builder_ptr);
  return 1;
}

void float_value_end_visit(const struct GraphQLAstFloatValue* node, void* builder_ptr) {
}

int int_value_begin_visit(const struct GraphQLAstIntValue* node, void* builder_ptr) {
  const char* str_int = GraphQLAstIntValue_get_value(node);
  VALUE rb_int = rb_funcall(rb_str_new2(str_int), rb_intern("to_i"), 0);
  assign_literal_value(rb_int, builder_ptr);
  return 1;
}

void int_value_end_visit(const struct GraphQLAstIntValue* node, void* builder_ptr) {
}

int boolean_value_begin_visit(const struct GraphQLAstBooleanValue* node, void* builder_ptr) {
  const int bool_value = GraphQLAstBooleanValue_get_value(node);
  VALUE rb_bool = bool_value ? Qtrue : Qfalse;
  assign_literal_value(rb_bool, builder_ptr);

  return 1;
}

void boolean_value_end_visit(const struct GraphQLAstBooleanValue* node, void* builder_ptr) {
}

int string_value_begin_visit(const struct GraphQLAstStringValue* node, void* builder_ptr) {
  const char* str_value = GraphQLAstStringValue_get_value(node);
  VALUE rb_string = rb_str_new2(str_value);
  rb_enc_associate_index(rb_string, rb_utf_8_enc);
  assign_literal_value(rb_string, builder_ptr);
  return 1;
}

void string_value_end_visit(const struct GraphQLAstStringValue* node, void* builder_ptr) {
}

int enum_value_begin_visit(const struct GraphQLAstEnumValue* node, void* builder_ptr) {
  VALUE assigns = rb_hash_new();
  const char* str_value;
  VALUE rb_string;
  str_value = GraphQLAstEnumValue_get_value(node);
  rb_string = rb_str_new2(str_value);
  rb_enc_associate_index(rb_string, rb_utf_8_enc);
  rb_hash_aset(assigns, name_sym, rb_string);

  build_rb_node((struct GraphQLAstNode*) node, "Enum", builder_ptr, assigns);
  return 1;
}

void enum_value_end_visit(const struct GraphQLAstEnumValue* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int list_value_begin_visit(const struct GraphQLAstListValue* node, void* builder_ptr) {
  build_rb_node((struct GraphQLAstNode*) node, "ListLiteral", builder_ptr, Qnil);
  return 1;
}

void list_value_end_visit(const struct GraphQLAstListValue* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int object_value_begin_visit(const struct GraphQLAstObjectValue* node, void* builder_ptr) {
  build_rb_node((struct GraphQLAstNode*) node, "InputObject", builder_ptr, Qnil);
  return 1;
}

void object_value_end_visit(const struct GraphQLAstObjectValue* node, void* builder_ptr) {
  end_visit(builder_ptr);
}

int object_field_begin_visit(const struct GraphQLAstObjectField* node, void* builder_ptr) {
  VALUE assigns = rb_hash_new();
  ASSIGN_NAME(assigns, node, GraphQLAstObjectField_get_name)
  build_rb_node((struct GraphQLAstNode*) node, "Argument", builder_ptr, assigns);
  return 1;
}

void object_field_end_visit(const struct GraphQLAstObjectField* node, void* builder_ptr) {
  end_visit(builder_ptr);
}
