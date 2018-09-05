#include <cstring>

#include "assembler.h"
#include "lexer.h"
#include "ccml.h"
#include "errors.h"
#include "instructions.h"


using namespace ccml;

void asm_stream_t::set_line(lexer_t &lexer, const token_t *t) {
  // insert into the line table
  const uint8_t *ptr = head(0);
  if (t) {
    line_table_[ptr] = t->line_no_;
  }
  else {
    line_table_[ptr] = lexer.stream_.line_number();
  }
}

int32_t asm_stream_t::get_line(const uint8_t *p) const {
  auto itt = line_table_.find(p);
  if (itt != line_table_.end()) {
    return itt->second;
  }
  return -1;
}

assembler_t::assembler_t(ccml_t &c, asm_stream_t &stream)
  : ccml_(c)
  , stream(stream)
{
}

void assembler_t::write8(const uint8_t v) {
  if (!stream.write8(v)) {
    ccml_.errors().program_too_large();
  }
}

void assembler_t::write32(const int32_t v) {
  if (!stream.write32(v)) {
    ccml_.errors().program_too_large();
  }
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
    write8(ins);
    break;
  default:
    assert(!"unknown instruction");
  }
}

int32_t *assembler_t::emit(instruction_e ins, int32_t v, const token_t *t) {
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
  case INS_GETV:
  case INS_SETV:
  case INS_GETVI:
  case INS_SETVI:
  case INS_GETG:
  case INS_SETG:
  case INS_GETGI:
  case INS_SETGI:
    write8(ins);
    write32(v);
    break;
  default:
    assert(!"unknown instruction");
  }
  // return the operand
  return (int32_t*)(stream.head(-4));
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
  case TOK_EQ:  emit(INS_EQ , t); break;
  case TOK_LT:  emit(INS_LT , t); break;
  case TOK_GT:  emit(INS_GT , t); break;
  case TOK_LEQ: emit(INS_LEQ, t); break;
  case TOK_GEQ: emit(INS_GEQ, t); break;
  default:
    assert(!"cant emit token type");
  }
}

int32_t assembler_t::pos() const {
  return stream.pos();
}

int32_t &assembler_t::get_fixup() {
  return *reinterpret_cast<int32_t*>(stream.head(-4));
}

void assembler_t::reset() {
}
