/*

Copyright (c) 2016 Louai Al-Khanji

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef NOLDOR_H
#define NOLDOR_H

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <stdexcept>
#include <cstdint>

#ifdef WIN32
# define NOLDOR_DECL_EXPORT __declspec(dllexport)
# define NOLDOR_DECL_IMPORT __declspec(dllimport)
#else
# define NOLDOR_DECL_EXPORT __attribute((visibility("default")))
# define NOLDOR_DECL_IMPORT __attribute((visibility("default")))
#endif

#ifdef BUILDING_NOLDOR
# define NOLDOR_EXPORT NOLDOR_DECL_EXPORT
#else
# define NOLDOR_EXPORT NOLDOR_DECL_IMPORT
#endif

#if defined(__GNUC__)
#  define NOLDOR_UNREACHABLE() __builtin_unreachable()
#endif

namespace noldor {

struct NOLDOR_EXPORT value
{
    value() = delete;
    explicit value(bool) = delete;

    inline bool operator==(const value &) = delete;
    bool operator!=(const value &) = delete;

    value(const value &) = default;
    value(value &&) = default;

    value &operator=(const value &) = default;
    value &operator=(value &&) = default;

    inline value(uint64_t u)
        : d(u)
    {}

    inline value &operator=(uint64_t u)
    { d = u; return *this; }

    inline operator uint64_t () const noexcept
    { return d; }

private:
    uint64_t d;
};

class noldor_exception : public std::exception
{
    std::string _msg;
public:
    noldor_exception(std::string msg);
    const char* what() const noexcept override;
};

class NOLDOR_EXPORT base_error : public noldor_exception
{
    value _irritants;
public:
    base_error(std::string msg, value irritants);
    value irritants() const noexcept;
};

class NOLDOR_EXPORT type_error : public base_error
{
public:
    using base_error::base_error;
};

class NOLDOR_EXPORT call_error : public base_error
{
public:
    using base_error::base_error;
};

class NOLDOR_EXPORT file_error : public base_error
{
public:
    using base_error::base_error;
};

class NOLDOR_EXPORT variable_error : public base_error
{
public:
    using base_error::base_error;
};

class NOLDOR_EXPORT runtime_error : public noldor_exception
{
public:
    using noldor_exception::noldor_exception;
};

class NOLDOR_EXPORT parse_error : public noldor_exception
{
public:
    using noldor_exception::noldor_exception;
};

inline void check_type(bool (*predicate)(value), value val, const char *msg)
{
    if (!predicate(val)) throw noldor::type_error(msg, val);
}

inline void check_type(bool (*predicate)(uint64_t), uint64_t val, const char *msg)
{
    if (!predicate(val)) throw noldor::type_error(msg, val);
}

constexpr uint8_t METATYPE_VERSION = 0;

typedef void (*gc_visit_fn_t)(value *child, void *data);

struct NOLDOR_EXPORT list_t {
    list_t *next = this;
    list_t *prev = this;
};

struct NOLDOR_EXPORT scope
{
    scope();
    virtual ~scope();
    virtual void visit(gc_visit_fn_t visitor, void *data) = 0;

    list_t gc_scopes;
};

enum typeflags {
    typeflags_none      = 0x0,
    typeflags_static    = 0x1, // GC should not reap
    typeflags_self_eval = 0x2, // evaluates to itself
};

struct NOLDOR_EXPORT metatype_t {
    const uint8_t version;
    const typeflags flags;

    void (* const destruct)(value self);
    void (* const gc_visit)(value self, gc_visit_fn_t visitor, void *data);
    std::string (* const repr)(value self);
};

NOLDOR_EXPORT void noldor_init(int argc, char **argv);
NOLDOR_EXPORT value allocate(metatype_t *metaobject, size_t size, size_t alignment = alignof(uintptr_t));

NOLDOR_EXPORT void register_function(const char *name, std::function<value(value)> fn);

NOLDOR_EXPORT metatype_t *object_metaobject(value obj);
NOLDOR_EXPORT void *object_data(value obj);

template <class T>
inline value object_allocate(metatype_t *metaobject, T &&obj)
{
    value cell = allocate(metaobject, sizeof(T), alignof(T));
    new (object_data(cell)) T(std::move(obj));
    return cell;
}

template <class T>
inline T object_data_as(value obj)
{
    return static_cast<T>(object_data(obj));
}

struct char_t {
    uint32_t character;
};

struct NOLDOR_EXPORT dot_tag {};

#define X_NOLDOR_SHARED_PROCEDURES(X) \
    X("eqv?",                       eqv,                        bool,           value, value                ) \
    X("eq?",                        eq,                         bool,           value, value                ) \
    X("equal?",                     equal,                      bool,           value, value                ) \
    X("number?",                    is_number,                  bool,           value                       ) \
    X("real?",                      is_double,                  bool,           value                       ) \
    X("integer?",                   is_int,                     bool,           value                       ) \
    X("=",                          num_eq,                     bool,           dot_tag, value              ) \
    X("<",                          num_st,                     bool,           dot_tag, value              ) \
    X(">",                          num_gt,                     bool,           dot_tag, value              ) \
    X("<=",                         num_ste,                    bool,           dot_tag, value              ) \
    X(">=",                         num_gte,                    bool,           dot_tag, value              ) \
    X("zero?",                      is_zero,                    bool,           value                       ) \
    X("positive?",                  is_positive,                bool,           value                       ) \
    X("negative?",                  is_negative,                bool,           value                       ) \
    X("odd?",                       is_odd,                     bool,           value                       ) \
    X("even?",                      is_even,                    bool,           value                       ) \
    X("max",                        max,                        value,          dot_tag, value              ) \
    X("min",                        min,                        value,          dot_tag, value              ) \
    X("+",                          add,                        value,          dot_tag, value              ) \
    X("-",                          sub,                        value,          dot_tag, value              ) \
    X("*",                          mul,                        value,          dot_tag, value              ) \
    X("/",                          div,                        value,          dot_tag, value              ) \
    X("boolean?",                   is_bool,                    bool,           value                       ) \
    X("not",                        is_false,                   bool,           value                       ) \
    X("pair?",                      is_pair,                    bool,           value                       ) \
    X("cons",                       cons,                       value,          value, value                ) \
    X("car",                        car,                        value,          value                       ) \
    X("cdr",                        cdr,                        value,          value                       ) \
    X("set-car!",                   set_car,                    value,          value, value                ) \
    X("set-cdr!",                   set_cdr,                    value,          value, value                ) \
    X("caar",                       caar,                       value,          value                       ) \
    X("cadr",                       cadr,                       value,          value                       ) \
    X("cdar",                       cdar,                       value,          value                       ) \
    X("cddr",                       cddr,                       value,          value                       ) \
    X("caaar",                      caaar,                      value,          value                       ) \
    X("caadr",                      caadr,                      value,          value                       ) \
    X("cadar",                      cadar,                      value,          value                       ) \
    X("caddr",                      caddr,                      value,          value                       ) \
    X("cdaar",                      cdaar,                      value,          value                       ) \
    X("cdadr",                      cdadr,                      value,          value                       ) \
    X("cddar",                      cddar,                      value,          value                       ) \
    X("cdddr",                      cdddr,                      value,          value                       ) \
    X("caaaar",                     caaaar,                     value,          value                       ) \
    X("caaadr",                     caaadr,                     value,          value                       ) \
    X("caadar",                     caadar,                     value,          value                       ) \
    X("caaddr",                     caaddr,                     value,          value                       ) \
    X("cadaar",                     cadaar,                     value,          value                       ) \
    X("cadadr",                     cadadr,                     value,          value                       ) \
    X("caddar",                     caddar,                     value,          value                       ) \
    X("cadddr",                     cadddr,                     value,          value                       ) \
    X("cdaaar",                     cdaaar,                     value,          value                       ) \
    X("cdaadr",                     cdaadr,                     value,          value                       ) \
    X("cdadar",                     cdadar,                     value,          value                       ) \
    X("cdaddr",                     cdaddr,                     value,          value                       ) \
    X("cddaar",                     cddaar,                     value,          value                       ) \
    X("cddadr",                     cddadr,                     value,          value                       ) \
    X("cdddar",                     cdddar,                     value,          value                       ) \
    X("cddddr",                     cddddr,                     value,          value                       ) \
    X("null?",                      is_null,                    bool,           value                       ) \
    X("list?",                      is_list,                    bool,           value                       ) \
    X("list",                       list,                       value,          dot_tag, value              ) \
    X("length",                     length,                     int32_t,        value                       ) \
    X("append",                     append,                     value,          value, value                ) \
    X("reverse",                    reverse,                    value,          value                       ) \
    X("list-tail",                  list_tail,                  value,          value, int32_t              ) \
    X("assq",                       assq,                       value,          value, value                ) \
    X("symbol?",                    is_symbol,                  bool,           value                       ) \
    X("symbol->string",             symbol_to_string,           std::string,    value                       ) \
    X("string->symbol",             symbol,                     value,          std::string                 ) \
    X("char?",                      is_char,                    bool,           value                       ) \
    X("string?",                    is_string,                  bool,           value                       ) \
    X("vector?",                    is_vector,                  bool,           value                       ) \
    X("procedure?",                 is_procedure,               bool,           value                       ) \
    X("primitive-procedure?",       is_primitive_procedure,     bool,           value                       ) \
    X("compound-procedure?",        is_compound_procedure,      bool,           value                       ) \
    X("apply",                      apply,                      value,          value, dot_tag, value       ) \
    X("environment?",               is_environment,             bool,           value                       ) \
    X("environment",                environment,                value,          value                       ) \
    X("null-environment",           null_environment,           value,          value                       ) \
    X("interaction-environment",    interaction_environment,    value,                                      ) \
    X("eval",                       eval,                       value,          value, value                ) \
    X("parametrize",                parametrize,                value,          value, dot_tag, value       ) \
    X("input-port?",                is_input_port,              bool,           value                       ) \
    X("output-port?",               is_output_port,             bool,           value                       ) \
    X("textual-port?",              is_textual_port,            bool,           value                       ) \
    X("binary-port?",               is_binary_port,             bool,           value                       ) \
    X("port?",                      is_port,                    bool,           value                       ) \
    X("input-port-open?",           is_input_port_open,         bool,           value                       ) \
    X("output-port-open?",          is_output_port_open,        bool,           value                       ) \
    X("current-input-port",         current_input_port,         value,                                      ) \
    X("current-output-port",        current_output_port,        value,                                      ) \
    X("current-error-port",         current_error_port,         value,                                      ) \
    X("file-port?",                 is_file_port,               bool,           value                       ) \
    X("open-input-file",            open_input_file,            value,          std::string                 ) \
    X("open-binary-input-file",     open_binary_input_file,     value,          std::string                 ) \
    X("open-output-file",           open_output_file,           value,          std::string                 ) \
    X("open-binary-output-file",    open_binary_output_file,    value,          std::string                 ) \
    X("close-port",                 close_port,                 bool,           value                       ) \
    X("close-input-port",           close_input_port,           bool,           value                       ) \
    X("close-output-port",          close_output_port,          bool,           value                       ) \
    X("string-port?",               is_string_port,             bool,           value                       ) \
    X("open-input-string",          open_input_string,          value,          std::string                 ) \
    X("open-output-string",         open_output_string,         value,                                      ) \
    X("get-output-string",          get_output_string,          std::string,    value                       ) \
    X("read",                       read,                       value,          dot_tag, value              ) \
    X("read-char",                  read_char,                  value,          dot_tag, value              ) \
    X("peek-char",                  peek_char,                  value,          dot_tag, value              ) \
    X("read-line",                  read_line,                  value,          dot_tag, value              ) \
    X("eof-object?",                is_eof_object,              bool,           value                       ) \
    X("eof-object" ,                mk_eof_object,              value,                                      ) \
    X("char-ready?",                is_char_ready,              bool,           dot_tag, value              ) \
    X("write",                      write,                      value,          value, dot_tag, value       ) \
    X("display",                    display,                    value,          value, dot_tag, value       ) \
    X("newline",                    newline,                    value,          dot_tag, value              ) \
    X("load",                       load,                       value,          std::string, dot_tag, value ) \
    X("file-exists?",               file_exists,                bool,           std::string                 ) \
    X("delete-file",                delete_file,                bool,           std::string                 ) \
    X("command-line",               command_line,               value,                                      ) \
    X("exit",                       exit,                       bool,           dot_tag, value              ) \
    X("emergency-exit",             emergency_exit,             bool,           dot_tag, value              ) \
    X("get-environment-variable",   get_environment_variable,   value,          std::string                 ) \
    X("get-environment-variables",  get_environment_variables,  value,                                      ) \
    X("external-representation",    printable,                  std::string,    value                       ) \
    X("current-second",             current_second,             double,                                     ) \
    X("current-jiffy",              current_jiffy,              int32_t,                                    ) \
    X("jiffies-per-second",         jiffies_per_second,         int32_t,                                    ) \
    X("tagged-list?",               is_tagged_list,             bool,           value, value                ) \
    X("garbage-collect",            run_gc,                     int,                                        )

#define DECLARE_C_FUNCTION(LISP_NAME, C_NAME, C_RETURN, ...) NOLDOR_EXPORT C_RETURN C_NAME (__VA_ARGS__);
X_NOLDOR_SHARED_PROCEDURES(DECLARE_C_FUNCTION)
#undef DECLARE_C_FUNCTION

#define SYMBOL_LITERAL(NAME) [] () -> value { static value sym = symbol(#NAME); return sym; } ()

NOLDOR_EXPORT value list();

template <class... Args>
inline value list(value v, Args... args)
{ return cons(v, list(args...)); }

NOLDOR_EXPORT value mk_bool(bool);

NOLDOR_EXPORT value mk_double(double);
NOLDOR_EXPORT double to_double(value);

NOLDOR_EXPORT value mk_int(int32_t);
NOLDOR_EXPORT int32_t to_int(value);

NOLDOR_EXPORT value mk_environment(value outer = list());
NOLDOR_EXPORT value mk_empty_environment();

NOLDOR_EXPORT value environment_global();
NOLDOR_EXPORT value environment_find(value env, value sym);
NOLDOR_EXPORT value environment_get(value env, value sym);
NOLDOR_EXPORT value environment_set(value env, value sym, value val);
NOLDOR_EXPORT value environment_define(value env, value sym, value val);

NOLDOR_EXPORT value mk_procedure(value parameters, value body, value environment);
NOLDOR_EXPORT value procedure_parameters(value);
NOLDOR_EXPORT value procedure_body(value);
NOLDOR_EXPORT value procedure_environment(value);

NOLDOR_EXPORT value mk_primitive_procedure(std::string name, value (*fptr)(value));
NOLDOR_EXPORT value apply_primitive_procedure(value proc, value argl);

NOLDOR_EXPORT value mk_vector(std::vector<value> elements);
NOLDOR_EXPORT std::vector<value> vector_get(value vec);

NOLDOR_EXPORT value mk_string(std::string);
NOLDOR_EXPORT std::string string_get(value);

NOLDOR_EXPORT value mk_char(uint32_t c);
NOLDOR_EXPORT uint32_t char_get(value);

NOLDOR_EXPORT value mk_port_from_fd(int fd, int oflag);
NOLDOR_EXPORT value read(value);
NOLDOR_EXPORT value read_char(value);
NOLDOR_EXPORT value peek_char(value);
NOLDOR_EXPORT value read_line(value);
NOLDOR_EXPORT bool is_char_ready(value port);

NOLDOR_EXPORT void set_command_line(int argc, char **argv);

NOLDOR_EXPORT std::ostream & operator << (std::ostream & os, value val);

struct basic_scope : scope
{
    basic_scope() = default;

    inline basic_scope(std::initializer_list<value *> vars)
        : variables(std::move(vars))
    {}

    std::vector<value *> variables;

    void visit(gc_visit_fn_t visitor, void *data) override
    {
        for (value *v : variables)
            visitor(v, data);
    }
};


} // namespace noldor

#endif // NOLDOR_H
