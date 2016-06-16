visitDispatchers[X86::LEA16r] = &IREmitter::visitLEA16r;
visitDispatchers[X86::LEA32r] = &IREmitter::visitLEA32r;
visitDispatchers[X86::LEA64r] = &IREmitter::visitLEA64r;
visitDispatchers[X86::LEA64_32r] = &IREmitter::visitLEA64_32r;