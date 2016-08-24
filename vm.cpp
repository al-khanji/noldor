#include "noldor_impl.h"
#include <cassert>
#include <numeric>

namespace noldor {

typedef uint32_t instruction;

enum class opcode : uint8_t {
    movk,  // assign from constant
    gotok, // goto constant
    gotor, // goto register
    save,
    restore,
};

struct linkage_t {
    enum { link_return, link_next, link_goto_label } type;
    std::string label_name;
};

struct constant_t {
     int16_t pos;
};

struct instruction_sequence_t {
    std::unordered_set<reg> needs;
    std::unordered_set<reg> modifies;
    value statements = list();
};

static instruction make_instruction(opcode op, reg target)
{
    return uint32_t(op) | (uint32_t(target) << 8);
}

static instruction make_instruction(opcode op, uint16_t data)
{
    return uint32_t(op) | (uint32_t(data) << 8);
}

static instruction make_instruction(opcode op, reg target, uint16_t data)
{
    return uint32_t(op) | (uint32_t(target) << 8) | (uint32_t(data) << 16);
}

static instruction_sequence_t compile_linkage(linkage_t linkage)
{
    switch (linkage.type) {
    case linkage_t::link_return:
        return instruction_sequence_t {
            { reg::continu },
            {},
            { list(symbol("goto"), list(symbol("reg"), symbol("continu"))) }
        };

    case linkage_t::link_next:
        return instruction_sequence_t {};

    case linkage_t::link_goto_label:
        return instruction_sequence_t {
            {},
            {},
            { list(symbol("goto"), symbol(linkage.label_name)) }
        };
    }
}

/*
(define (preserving regs seq1 seq2)
  (if (null? regs)
      (append-instruction-sequences seq1 seq2)
      (let ((first-reg (car regs)))
        (if (and (needs-register? seq2 first-reg)
                 (modifies-register? seq1 first-reg))
            (preserving (cdr regs)
             (make-instruction-sequence
              (list-union (list first-reg)
                          (registers-needed seq1))
              (list-difference (registers-modified seq1)
                               (list first-reg))
              (append `((save ,first-reg))
                      (statements seq1)
                      `((restore ,first-reg))))
             seq2)
            (preserving (cdr regs) seq1 seq2)))))
*/

static bool needs_register(reg r, const instruction_sequence_t &seq)
{
    return seq.needs.find(r) != seq.needs.end();
}

static bool modifies_register(reg r, const instruction_sequence_t &seq)
{
    return seq.modifies.find(r) != seq.modifies.end();
}
static instruction_sequence_t append_two_sequences(const instruction_sequence_t &seq1,
                                                   const instruction_sequence_t &seq2)
{
    return instruction_sequence_t {
        list_union(seq1.needs, list_difference(seq1.modifies, seq2.needs)),
        list_union(seq1.modifies, seq2.modifies),


    };
}

static instruction_sequence_t append_instruction_sequences(std::vector<instruction_sequence_t> seqs)
{
    return std::accumulate(seqs.begin(), seqs.end(), instruction_sequence_t {}, append_two_sequences);
}

static instruction_sequence_t preserving(std::vector<reg> regs, instruction_sequence_t seq1, instruction_sequence_t seq2)
{
    if (regs.empty())
        return append_instruction_sequences({seq1, seq2});

    const reg r = regs.back();
    regs.pop_back();

    if (needs_register(r, seq2) && modifies_register(r, seq1)) {
        seq1.needs.insert(r);
        seq1.modifies.erase(r);

        seq1.statements = append(list(symbol("restore"), symbol(regnames[int(r)])), seq1.statements);
        seq1.statements = append(seq1.statements, list(symbol("restore"), symbol(regnames[int(r)])));
    }

    return preserving(regs, seq1, seq2);
}

static instruction_sequence_t end_with_linkage(linkage_t linkage, instruction_sequence_t insseq)
{
    return preserving({reg::continu}, insseq, compile_linkage(linkage));
}

static bool is_self_eval(value v)
{ return is_int(v) || is_double(v) || is_string(v) || is_char(v); }

instruction_sequence_t compile_self_eval(value exp, reg target, linkage_t linkage)
{
    return end_with_linkage(linkage,
                            instruction_sequence_t { {},
                                                     { target },
                                                     { list(symbol("assign"), exp) }});
}

instruction_sequence_t compile(value exp, reg target, linkage_t linkage)
{
    if (is_self_eval(exp))
        return compile_self_eval(exp, target, linkage);

    assert(false);
}

} // namespace noldor
