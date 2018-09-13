#include <cstring>

#include "assembler.h"
#include "disassembler.h"
#include "lexer.h"
#include "ccml.h"
#include "errors.h"
#include "instructions.h"
#include "parser.h"


using namespace ccml;

namespace {

bool has_operand(const instruction_e ins) {
  switch (ins) {
  case INS_JMP:
  case INS_CJMP:
  case INS_CALL:
  case INS_RET:
  case INS_SCALL:
  case INS_POP:
  case INS_CONST:
  case INS_LOCALS:
  case INS_ACCV:
  case INS_GETV:
  case INS_SETV:
  case INS_GETVI:
  case INS_SETVI:
  case INS_GETG:
  case INS_SETG:
  case INS_GETGI:
  case INS_SETGI:
    return true;
  default:
    return false;
  }
}

} // namespace

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
void asm_stream_t::set_line(lexer_t &lexer, const token_t *t) {
  const uint32_t pc = pos();
  const uint32_t line = t ? t->line_no_ : lexer.stream_.line_number();
  store_.add_line(pc, line);
}

void asm_stream_t::add_ident(const identifier_t &ident) {
  for (auto &i : store_.identifiers_) {
    if (i.token == ident.token) {
      i.start = std::min(ident.start, i.start);
      i.end   = std::max(ident.end,   i.end);
      return;
    }
  }
  store_.identifiers_.push_back(ident);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
assembler_t::assembler_t(ccml_t &c, asm_stream_t &stream)
  : ccml_(c)
  , stream(stream)
{
}

void assembler_t::add_ident(const identifier_t &ident) {
//  printf("%03d -> %03d  %s\n", ident.start, ident.end, ident.token->string());
  stream.add_ident(ident);
}

void assembler_t::emit(instruction_e ins, const token_t *t) {
  stream.set_line(ccml_.lexer(), t);
  // encode this instruction
  switch (ins) {
  case INS_ADD:
  case INS_SUB:
  case INS_MUL:
  case INS_DIV:
  case INS_MOD:
  case INS_AND:
  case INS_OR:
  case INS_NOT:
  case INS_LT:
  case INS_GT:
  case INS_LEQ:
  case INS_GEQ:
  case INS_EQ:
  case INS_NEG:
    peep_hole_.push_back(instruction_t{ins, 0, t});
    break;
  default:
    assert(!"unknown instruction");
  }
}

void assembler_t::emit(instruction_e ins, int32_t v, const token_t *t) {
  stream.set_line(ccml_.lexer(), t);
  // encode this instruction
  switch (ins) {
  case INS_JMP:
  case INS_CJMP:
  case INS_CALL:
  case INS_RET:
  case INS_SCALL:
  case INS_POP:
  case INS_CONST:
  case INS_LOCALS:
  case INS_ACCV:
  case INS_GETV:
  case INS_SETV:
  case INS_GETVI:
  case INS_SETVI:
  case INS_GETG:
  case INS_SETG:
  case INS_GETGI:
  case INS_SETGI:
    peep_hole_.push_back(instruction_t{ins, v, t});
    break;
  default:
    assert(!"unknown instruction");
  }
}

void assembler_t::emit(token_e tok, const token_t *t) {
  switch (tok) {
  case TOK_ADD: emit(INS_ADD, t); break;
  case TOK_SUB: emit(INS_SUB, t); break;
  case TOK_MUL: emit(INS_MUL, t); break;
  case TOK_DIV: emit(INS_DIV, t); break;
  case TOK_MOD: emit(INS_MOD, t); break;
  case TOK_AND: emit(INS_AND, t); break;
  case TOK_OR:  emit(INS_OR , t); break;
  case TOK_NOT: emit(INS_NOT, t); break;
  case TOK_NEG: emit(INS_NEG, t); break;
  case TOK_EQ:  emit(INS_EQ , t); break;
  case TOK_LT:  emit(INS_LT , t); break;
  case TOK_GT:  emit(INS_GT , t); break;
  case TOK_LEQ: emit(INS_LEQ, t); break;
  case TOK_GEQ: emit(INS_GEQ, t); break;
  default:
    assert(!"cant emit token type");
  }
}

int32_t assembler_t::pos() {
  peep_hole_flush_();
  return stream.pos();
}

int32_t *assembler_t::get_fixup() {
  assert(!peep_hole_.empty());
  const instruction_t last = peep_hole_.back();
  peep_hole_.pop_back();
  peep_hole_flush_();
  emit_(last);
  return reinterpret_cast<int32_t*>(stream.head(-4));
}

void assembler_t::reset() {
  peep_hole_.clear();
  // stream.reset() ?
}

// emit an instruction into the instruction stream
void assembler_t::emit_(const instruction_t &ins) {
  stream.set_line(ccml_.lexer(), ins.token);
  if (!stream.write8(ins.opcode)) {
    ccml_.errors().program_too_large();
  }
  if (has_operand(ins.opcode)) {
    if (!stream.write32(ins.operand)) {
      ccml_.errors().program_too_large();
    }
  }
}

void assembler_t::flush() {
  peep_hole_flush_();
}

void assembler_t::peep_hole_flush_() {

  // nothing to do
  if (peep_hole_.empty()) {
    return;
  }

  // lets do some optimizations
  peep_hole_optimize_();

  // flush all instruction to the output stream
  for (const instruction_t &ins : peep_hole_) {
    emit_(ins);
  }

  // peephole is empty now
  peep_hole_.clear();
}
