# Loop del world e MapUpdater

**Stato:** Implementato

`World::Update` aggiorna sessioni, mappe e callback DB.  
`MapUpdater` usa una coda e N thread worker; non parallelizzare in modo unsafe lo stesso tick di mappa.

**Sorgenti:** `World.cpp`, `MapUpdater.*`, `MapManager.*`.
