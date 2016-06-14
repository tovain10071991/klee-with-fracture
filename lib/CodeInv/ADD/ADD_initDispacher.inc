visitDispachers[X86::ADD32rr] = &IREmitter::visitADD32rr;
visitDispachers[X86::ADD64rr] = &IREmitter::visitADD64rr;
visitDispachers[X86::ADD32ri8] = &IREmitter::visitADD32ri8;
visitDispachers[X86::ADD64ri8] = &IREmitter::visitADD64ri8;
visitDispachers[X86::ADD64ri32] = &IREmitter::visitADD64ri32;
visitDispachers[X86::ADD64i32] = &IREmitter::visitADD64i32;