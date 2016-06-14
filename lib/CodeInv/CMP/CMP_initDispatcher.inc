visitDispatchers[X86::CMP32ri8] = &IREmitter::visitCMP32ri8;
visitDispatchers[X86::CMP64ri8] = &IREmitter::visitCMP64ri8;
visitDispatchers[X86::CMP64i32] = &IREmitter::visitCMP64i32;
visitDispatchers[X86::CMP64rr] = &IREmitter::visitCMP64r;
visitDispatchers[X86::CMP32mi8] = &IREmitter::visitCMP32mi8;
visitDispatchers[X86::CMP64mi8] = &IREmitter::visitCMP64mi8;
visitDispatchers[X86::CMP8mi] = &IREmitter::visitCMP8mi;
visitDispatchers[X86::CMP64rm] = &IREmitter::visitCMP64rm;