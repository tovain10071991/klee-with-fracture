visitDispatchers[X86::CALL64pcrel32] = &IREmitter::visitCALL64pcrel32;
visitDispatchers[X86::CALL64r] = &IREmitter::visitCALL64r;
visitDispatchers[X86::CALL64m] = &IREmitter::visitCALL64m;