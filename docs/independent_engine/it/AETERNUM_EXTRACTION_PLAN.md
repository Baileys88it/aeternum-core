# Piano di estrazione Aeternum

> Versione italiana. EN: [`../en/AETERNUM_EXTRACTION_PLAN.md`](../en/AETERNUM_EXTRACTION_PLAN.md)

**Stato:** allineato a OWNER_DECISIONS 2026-11-05  
**Metodo:** strangler incrementale dietro interfacce sotto **GPL-2.0**. Niente rewrite big-bang. Clean-room solo per modulo con guadagno **misurato**, con corpus pacchetti golden + replay per dimostrare parità.

## Ordine confermato
1. **Protocol** — profili Forever, ExtraInserts, remap, poi codec tipizzati  
2. **Operations** — Metric local sink + Admin HTTP/Prometheus dietro porte  
3. **Module** — SDK host Trinity-free sulla superficie `aeternum/api`  
4. **Kernel** — façade lifecycle boot/drain/shutdown  
5. **World/Gameplay** — solo dopo i boundary sopra; moduli gameplay dopo estrazione World

## Regole operative
- Ogni passo è piccolo, reversibile, misurato (Catch2 + goldens + soak dove applicabile).  
- API pubbliche solo con consumatore reale in-tree.  
- Trinity produzione resta fino a fine soak + 14 giorni giocatori + ok esplicito.  
- Soak Aeternum: non fermare/sovrascrivere binari RelWithDebInfo in uso; build in albero separato.

## Criterio di avanzamento
Un pezzo si considera estratto quando:
1. Esiste un contratto pubblico senza tipi Trinity vietati  
2. Un adapter sottile delega all’implementazione attuale  
3. I test di parità (goldens/unit) passano  
4. Nessuna regressione intenzionale sul wire Forever
