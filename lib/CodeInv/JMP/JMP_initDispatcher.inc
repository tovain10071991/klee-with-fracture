visitDispatchers[X86::JMP64r] = &IREmitter::visitJMP64r;
visitDispatchers[X86::JMP_1] = &IREmitter::visitJMP;
visitDispatchers[X86::JMP64pcrel32] = &IREmitter::visitJMP;