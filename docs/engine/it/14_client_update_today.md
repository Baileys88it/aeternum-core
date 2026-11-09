# Update del client Forever — procedura ODIERNA

**Stato:** Parzialmente implementato (manuale)

1. Nuova build + extract.  
2. `build_info` + `realmlist.gamebuild`.  
3. Aggiornare ExtraInserts **solo dopo sniff** (non copiare alla cieca da PR 70755).  
4. Sistemare layout packet se serve.  
5. Probe vuoti → stub.  
6. SQL/hotfixes con rollback.  
7. Build locale; riavvio Trinity solo se maintainer lo chiede (avviso 120s).  
8. Validare login, **creazione personaggio**, who/party/arena.  
9. NOTES in `sql/custom`.

Direzione futura (non design M0): profili client + test automatici — priorità #1 con Module SDK.
