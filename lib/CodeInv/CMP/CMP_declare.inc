#define declare_cmp(name) \
  void Cmp##name(BasicBlock *BB, MachineInstr* I, unsigned init_size, unsigned final_size)

declare_cmp(r_);
declare_cmp(rm);
declare_cmp(mi);

declare_visit(CMP8rr);
// declare_visit(CMP8rr_REV);
declare_visit(CMP16rr);
// declare_visit(CMP16rr_REV);
// declare_visit(CMP32rr_REV);
declare_visit(CMP32rr);
declare_visit(CMP64rr);
declare_visit(CMP8ri);
// declare_visit(CMP8i8);
declare_visit(CMP16ri);
declare_visit(CMP16ri8);
// declare_visit(CMP16i16);
// declare_visit(CMP32i32);
declare_visit(CMP32ri);
declare_visit(CMP32ri8);
declare_visit(CMP64ri32);
declare_visit(CMP64ri8);
declare_visit(CMP64i32);
declare_visit(CMP32mi8);
declare_visit(CMP8mi);
declare_visit(CMP16mi);
declare_visit(CMP32mi);
declare_visit(CMP8rm);
declare_visit(CMP16rm);
declare_visit(CMP32rm);
// declare_visit(CMP8mr);
// declare_visit(CMP16mr);
// declare_visit(CMP32mr);
declare_visit(CMP16mi8);
declare_visit(CMP64mi8);
declare_visit(CMP64mi32);
declare_visit(CMP64rm);
// declare_visit(CMP64mr);