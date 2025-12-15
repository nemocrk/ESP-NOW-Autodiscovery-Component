```markdown
# 📡 ESPHome Mesh Architect | ESP-NOW Native Mesh

![ESPHome Version](https://img.shields.io/badge/ESPHome-2024.x-green.svg)
![Platform](https://img.shields.io/badge/Platform-ESP32-blue.svg)
![License](https://img.shields.io/badge/License-MIT-orange.svg)

Un framework avanzato per creare reti Mesh **Zero-Config** utilizzando il protocollo **ESP-NOW** nativo su ESPHome.

Questo progetto include due parti:
1. **Mesh Architect Generator (`EspNowApp.html`)**: Un tool grafico standalone per generare il codice del componente.
2. **Native Mesh Component**: Il codice C++/Python sottostante che gestisce il protocollo di rete.

---

## ✨ Caratteristiche Principali

*   **⚡ Zero-Config Nodes**: I nodi sensori **non** richiedono configurazione WiFi (SSID/Password) né configurazione del canale nel YAML. Basta flasharli e accenderli.
*   **🔍 Auto-Scan & Channel Locking**: I nodi scansionano automaticamente i canali (1-13) al boot, trovano il Root e si agganciano dinamicamente.
*   **🛠️ Bare Metal Initialization**: I nodi utilizzano chiamate dirette ESP-IDF per inizializzare la radio. Il pesante componente standard `wifi:` di ESPHome viene completamente rimosso dai nodi per risparmiare risorse e velocizzare il boot.
*   **🔒 Sicurezza Dinamica (LMK)**: Utilizza una Master Key (PMK) per l'handshake iniziale, ma deriva chiavi di sessione univoche (LMK) per ogni link crittografato.
*   **🏠 Home Assistant Discovery**: Il Root agisce da bridge MQTT trasparente. Le entità dei nodi vengono rilevate tramite *Introspezione* e registrate automaticamente in Home Assistant come dispositivi separati.
*   **🛡️ Safe Peer Management (LRU)**: Include un gestore della tabella dei peer che previene i crash dell'ESP32 (limite hardware <20 peer) ruotando automaticamente i dispositivi attivi.
*   **🌐 Routing Ibrido Layer 3**: Supporta routing multi-hop con auto-apprendimento del percorso di ritorno (Reverse Path Learning).

---

## 🚀 Guida Rapida: Usare il Generatore

Al link https://nemocrk.github.io/ESP-NOW-Autodiscovery-Component/ trovi una Single-Page Application che contiene tutta la logica necessaria per creare i file del componente.

1.  Scarica il file `EspNowApp.html`.
2.  Aprilo con un qualsiasi browser moderno (Chrome, Firefox, Edge).
3.  Configura i parametri della tua rete:
    *   **Ruolo:** Scegli tra `ROOT` (Gateway) o `NODE` (Sensore).
    *   **Mesh ID:** Un identificativo univoco per la tua rete.
    *   **PMK:** Una chiave di sicurezza (esattamente 16 caratteri).
4.  Clicca su **"Copia Codice Attivo"**.
5.  Il tool genererà 4 file: `config.yaml`, `__init__.py`, `mesh.h`, `mesh.cpp`.

---

## 📂 Installazione Manuale del Componente

Per utilizzare il codice generato nel tuo progetto ESPHome, segui questa struttura di directory:

```text
config/
├── components/
│   └── esp_mesh/          <-- Crea questa cartella
│       ├── __init__.py    <-- Codice Python generato
│       ├── mesh.h         <-- Header C++ generato
│       └── mesh.cpp       <-- Sorgente C++ generato
├── secrets.yaml
├── gateway.yaml           <-- Configurazione ROOT
└── sensore_sala.yaml      <-- Configurazione NODE
```

### 1. Configurazione ROOT (Gateway)
Il Root deve avere accesso al WiFi e al Broker MQTT.

```yaml
esphome:
  name: mesh-gateway

esp32:
  board: esp32-devkitc-v4
  framework:
    type: esp-idf

# Il ROOT richiede WiFi standard e MQTT
wifi:
  ssid: "TuoSSID"
  password: "TuaPassword"

mqtt:
  broker: "192.168.1.10"
  topic_prefix: mesh_gw

# Carica il componente locale
external_components:
  - source: {type: local, path: components}

esp_mesh:
  mode: ROOT
  mesh_id: "MySmartMesh"
  pmk: "SecretKey1234567"  # Esattamente 16 chars
```

### 2. Configurazione NODE (Sensore)
Il Nodo è pulito. **Non definire** il blocco `wifi:` o `api:`.

```yaml
esphome:
  name: mesh-node-01

esp32:
  board: esp32-devkitc-v4
  framework:
    type: esp-idf

external_components:
  - source: {type: local, path: components}

esp_mesh:
  mode: NODE
  mesh_id: "MySmartMesh"
  pmk: "SecretKey1234567"

# Definisci i sensori normalmente. Il Mesh li troverà da solo.
sensor:
  - platform: dht
    pin: 23
    temperature:
      name: "Temperatura Sala"
    humidity:
      name: "Umidità Sala"
    update_interval: 60s
```

---

## 🧠 Dettagli Tecnici

### Inizializzazione "Bare Metal"
Nei nodi, il generatore Python rimuove la dipendenza `wifi` standard. Il file `mesh.cpp` utilizza le API di basso livello (`esp_wifi_init`, `esp_wifi_set_promiscuous`, ecc.) per attivare la radio in modalità Station senza connettersi ad alcun AP. Questo riduce drasticamente l'uso di RAM e Flash.

### Protocollo di Handshake
1.  **Probe:** Il nodo cicla i canali e invia un `PKT_PROBE` broadcast.
2.  **Announce:** Il Root (o un Repeater) risponde con `PKT_ANNOUNCE` contenente il suo Hop Count.
3.  **Lock & Key:** Il nodo si ferma sul canale, registra il mittente come genitore e deriva la LMK (`PMK XOR ParentMAC`) per cifrare le comunicazioni future.

### Introspezione (Reflection)
Il componente itera automaticamente su `App.get_sensors()`, `App.get_binary_sensors()`, etc. Non è necessario mappare manualmente quali sensori inviare. Ogni sensore definito nel YAML del nodo viene registrato sul Root e appare su Home Assistant.

### Safe Peer LRU (Least Recently Used)
L'ESP32 ha un limite hardware di peer cifrati (Max 17, raccomandato <10 per stabilità).
Questo componente implementa una coda LRU: se la tabella è piena, il peer che non comunica da più tempo viene rimosso per fare spazio al nuovo, garantendo che il gateway non si blocchi mai, anche con reti >20 nodi.

---

## ⚠️ Requisiti e Limitazioni

*   **Hardware:** Richiede ESP32 (ESP8266 non supportato da questa versione nativa).
*   **PMK:** La Master Key deve essere obbligatoriamente di **16 caratteri**.
*   **Canali:** Il Root determina il canale della mesh (basandosi sul WiFi a cui è connesso). I nodi lo seguono.
*   **Compilazione:** Al primo avvio, ESPHome compilerà l'intero framework ESP-IDF. Potrebbe richiedere tempo.

---

## 🤝 Contribuire

Sentiti libero di aprire Issue o Pull Request per migliorare l'algoritmo di routing o aggiungere supporto per nuovi tipi di entità (es. Climate, Light).

## 📄 Licenza

Distribuito sotto licenza MIT. Vedi `LICENSE` per maggiori informazioni.
```
