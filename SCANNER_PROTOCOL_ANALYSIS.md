# Analisi Comunicazione USB - Scanner LIDE25
## Protocollo di Avvio Scansione

### Panoramica Generale
La comunicazione con lo scanner LIDE25 avviene tramite trasferimenti BULK su endpoint USB (0x01 OUT / 0x81 IN). La cattura contiene 13.171 trasferimenti bidirezionali analizzati su oltre 184.000 frame.

---

## Modalità di Avvio Scansione

### Fase 1: Inizializzazione (Pattern Ripetuto)
**Sequenza base che si ripete ogni ~1 secondo:**

#### Operazione 1: Status Request
- **Comando OUT**: `01070001` (4 byte)
- **Endpoint**: 0x01 OUT
- **Risposta IN**: `00` (1 byte)
- **Significato**: Richiesta di stato del dispositivo (prob. check se pronto)
- **Occorrenze**: 2.868 volte

#### Operazione 2: Configurazione/Setup
- **Comando OUT**: `01020001` (4 byte)
- **Endpoint**: 0x01 OUT  
- **Risposta IN**: `63` (1 byte, valore esadecimale)
- **Significato**: Probabilmente comando di configurazione/setup
- **Occorrenze**: 409 volte

### Fase 2: Scansione Vera e Propria

#### Comandi Principali di Controllo:
| Comando | Bytes | Occorrenze | Probabilmente |
|---------|-------|-----------|---------------|
| `01070001` | 4 | 2.868 | Status check / Heartbeat |
| `01010001` | 4 | 2.666 | Comando scan/movimento |
| `02` | 1 | 2.043 | Comando generico |
| `00` | 1 | 1.152 | Reset/Stop |
| `05` | 1 | 1.105 | Status response |
| `03` | 1 | 924 | Comando configurazione |
| `01000c00` | 4 | 861 | Comando parametri estesi |
| `01000400` | 4 | 435 | Comando parametri |
| `63` | 1 | 371 | Risposta/Feedback |

### Fase 3: Trasferimento Dati Immagine

#### Comandi Dati:
| Comando | Bytes | Occorrenze | Descrizione |
|---------|-------|-----------|-------------|
| `0007000100` | 5 | 271 | Header trasferimento dati |
| `01000800` | 4 | 62 | Comando dati |
| `005700011f` | 5 | 83 | Dati con parametri |
| `0100ffff` | 4 | 74 | Feedback dati |
| `0005000100` | 5 | 108 | Comando generico |

### Fase 4: Terminazione

#### Comandi Finali:
| Comando | Bytes | Occorrenze |
|---------|-------|-----------|
| `01` | 1 | 171 | Fine comando |
| `ffffffff...` | 16 | 147 | Padding/Terminazione |

---

## Struttura Completa di una Scansione

```
1. LOOP di STATUS (ogni 1 secondo):
   OUT: 01070001 → IN: 00 (è pronto?)
   OUT: 01020001 → IN: 63 (configurazione OK)
   OUT: 01070001 → IN: 00

2. SETUP SCANSIONE:
   OUT: 01000c00 (parametri scan)
   OUT: 01000400 (parametri aggiuntivi)
   OUT: 03 (abilita scan)

3. TRASFERIMENTO DATI:
   OUT: 0007000100 (header dati)
   OUT: 005700011f (parametri trasferimento)
   [... stream dati immagine ...]

4. TERMINAZIONE:
   OUT: 00 (stop)
   OUT: 01 (fine)
   OUT: ffffffff... (padding)
```

---

## Parametri Chiave

- **Intervallo polling**: ~1 secondo tra cicli di status
- **Endpoint OUT**: 0x01 (host → dispositivo)
- **Endpoint IN**: 0x81 (dispositivo → host)
- **Tipo trasferimento**: BULK
- **Lunghezza comandi**: da 1 a 5 byte
- **Dimensione risposte**: tipicamente 1 byte

---

## Note Implementative

1. **Handshake continuo**: Il dispositivo richiede polling costante con comando `01070001` per verificare lo stato
2. **Configurazione statica**: I parametri di scan (`01000c00`, `01000400`) rimangono gli stessi
3. **Trasferimento streaming**: I dati immagine arrivano come stream continuo dopo l'inizio scansione
4. **Timeout**: Importante implementare timeout su risposte IN per gestire disconnessioni

---

## Implementazione Consigliata

```c
// Pseudo-codice per avvio scansione

1. Loop di polling:
   while (scanner_busy) {
       send_command(0x01, {0x01, 0x07, 0x00, 0x01});
       receive_response(0x81, &status);  // Aspetta 00
       if (status == READY) break;
       sleep(100ms);
   }

2. Setup parametri:
   send_command(0x01, {0x01, 0x00, 0x0c, 0x00});
   send_command(0x01, {0x01, 0x00, 0x04, 0x00});

3. Avvia scansione:
   send_command(0x01, {0x03});

4. Leggi dati immagine:
   while (!scan_complete) {
       bulk_read(0x81, buffer, MAX_SIZE);
       process_image_data(buffer);
   }

5. Termina:
   send_command(0x01, {0x00});  // Stop
   send_command(0x01, {0x01});  // End
```

---

**Generato da analisi PCAP**: lide25.pcap  
**Dispositivo**: Canon LiDE25 Scanner USB
