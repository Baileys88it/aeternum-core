# Architettura threading (M1)

**Stato:** Proposta  
**EN:** [../en/THREADING_ARCHITECTURE.md](../en/THREADING_ARCHITECTURE.md)

Preservare MapUpdater (coda + worker; ownership del tick di una mappa). World::Update orchestra sessioni, schedule mappe e ProcessQueryCallbacks. Misurare in M3 prima di ogni redesign. Niente thread-per-modulo per il pilota admin. Allocator custom solo con profile di allocazione. Rischio: callback modulo che tocca oggetti world fuori thread.

